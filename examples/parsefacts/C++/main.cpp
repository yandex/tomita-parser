#include <iostream>
#include <fstream>
#include <fcntl.h>

// Заголовочные файлы из Google Protobuf
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>

// Сгенерированный из facts.proto заголовочный файл с классами NFactex::*
#include "facts.pb.h"

using namespace std;
using namespace google::protobuf;
using namespace google::protobuf::io;

NFactex::TDocument document;

int main(int argc, char** argv) {

  int fd = open(argv[1], O_RDONLY);

  ZeroCopyInputStream* raw_input = new FileInputStream(fd);
  CodedInputStream* coded_input = new CodedInputStream(raw_input);

  uint32 size;
  while (coded_input->ReadVarint32(&size)) {
    CodedInputStream::Limit limit = coded_input->PushLimit(size);

    if (!document.ParseFromCodedStream(coded_input)) {
      cerr << "can't parse" << endl;
    }

    coded_input->PopLimit(limit);

    if (document.has_id())
      cout << "Document Id   = " << document.id() << endl;

    if (document.has_date())
      cout << "Document Date = " << document.date() << endl;

    if (document.has_url())
      cout << "Document URL  = " << document.url() << endl;

    // Факты внутри одного документа объединены в группы
    // фактов по их типу
    for (int i = 0; i < document.factgroup_size(); i++) {
      const NFactex::TFactGroup& fg = document.factgroup(i);
      cout << "\tFact Type = \"" << fg.type() << "\"" << endl;

      // Отдельные факты внутри группы
      for (int j = 0; j < fg.fact_size(); j++) {
        const NFactex::TFact& fact = fg.fact(j);

        cout << "\t\tindex = " << fact.index() << endl;

        // Атрибуты факта
        const NFactex::TFactAttr& attr = fact.attr();
        cout << "\t\t"
             << "leadindex="      << attr.leadindex() << " "
             << "sentencenumber=" << attr.sentencenumber() << " "
             << "textpos="        << attr.textpos() << " "
             << "textlen="        << attr.textlen() << " " << endl;

        if (attr.leadspanindex_size() > 0) {
          cout << "\t\tleadspanindex=";

          for (int k = 0; k < attr.leadspanindex_size(); k++) {
            cout << attr.leadspanindex(k);

            if (k < attr.leadspanindex_size()-1)
              cout << ", ";
          }

          cout << endl;
        }

        // Поля факта
        for (int k = 0; k < fact.field_size(); k++) {
          const NFactex::TFactField& field = fact.field(k);

          cout << "\t\t\t" << field.name() << ": ";

          if (field.has_value())
            cout << "\"" << field.value() << "\"" << endl;
          else
            cout << "\"\"" << endl;

          if (field.has_info())
            cout << "\t\t\t\tInfo=\"" << field.info() << "\"" << endl;

          for (int p = 0; p < field.option_size(); p++)
            cout << "\t\t\t\tOption #" << p << "=\"" << field.option(p) << "\"" << endl;
        }
      }
    }

    // Лиды - предложения, из которых были извлечены факты
    for (int i = 0; i < document.lead_size(); i++) {
      const NFactex::TLead& lead = document.lead(i);

      cout << "\tLead #" << i << endl;
      cout << "\t\tText = \"" << lead.text() << "\"" << endl;

      if (lead.has_docid())
        cout << "\t\tdocid=" << lead.docid() << endl;

      // Отрезки лидов (спаны)
      for (int j = 0; j < lead.span_size(); j++) {
        const NFactex::TLeadSpan& span = lead.span(j);

        cout << "\t\t\tSpan #" << j << endl;
        cout << "\t\t\tStartChar=" << span.startchar() << " StopChar=" << span.stopchar() << endl;
        cout << "\t\t\tStartByte=" << span.startbyte() << " StopByte=" << span.stopbyte() << endl;

        if (span.has_lemma())
          cout << "\t\t\tLemma=\"" << span.lemma() << "\"" << endl;

        if (span.has_typeletter())
          cout << "\t\t\tTypeLetter=\"" << span.typeletter() << "\"" << endl;
      }
    }

    cout << endl;
  }

  delete coded_input;
  delete raw_input;
  close(fd);

  return 0;
}
