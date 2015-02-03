// The following code includes modified parts of the protobuf library
// available at http://code.google.com/p/protobuf/
// under the terms of the BSD 3-Clause License
// http://opensource.org/licenses/BSD-3-Clause

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// Recursive descent FTW.

#include <compiler/cpp/cpp_helpers.h>
#include <io/zero_copy_stream.h>
#include <io/printer.h>
#include <stubs/strutil.h>
#include <stubs/common.h>
#include <descriptor.h>
#include <descriptor.pb.h>

#include "cpp_styleguide.h"
#include <util/stream/ios.h>

namespace NProtobuf {
namespace NCompiler {
namespace NPlugins {

    using namespace google::protobuf;
    using namespace google::protobuf::compiler;
    using namespace google::protobuf::compiler::cpp;

    typedef map<TProtoStringType, TProtoStringType> TVariables;

    void SetCommonFieldVariables(const FieldDescriptor* descriptor, TVariables* variables) {
        (*variables)["rname"] = descriptor->name();
        (*variables)["name"] = FieldName(descriptor);
#if 0
        /* doesn't work when there are different fields named xxx_yyy and XxxYyy
         * need to pass a per-message field registry to avoid overload clashes */
        TProtoStringType RName = descriptor->camelcase_name();
        RName.replace(0, 1, 1, toupper(RName[0]));
        if (descriptor->name() != RName)
            (*variables)["RName"] = RName;
#endif
    }

    TProtoStringType HeaderFileName(const FileDescriptor* file) {
        TProtoStringType basename = StripProto(file->name());

        return basename.append(".pb.h");
    }

    TProtoStringType SourceFileName(const FileDescriptor* file) {
        TProtoStringType basename = StripProto(file->name());

        return basename.append(".pb.cc");
    }

    class TFieldExtGenerator {
        public:
            TFieldExtGenerator(const FieldDescriptor* field)
                : Field_(field)
            {
                SetCommonFieldVariables(Field_, &Variables_);
            }

            virtual ~TFieldExtGenerator() throw () {
            }

            virtual void GenerateAccessorDeclarations(io::Printer* printer) = 0;

        protected:
            const FieldDescriptor* Field_;
            TVariables Variables_;
    };


    class TMessageFieldExtGenerator: public TFieldExtGenerator  {
        public:
            TMessageFieldExtGenerator(const FieldDescriptor* field)
                : TFieldExtGenerator(field)
            {
            }

            void GenerateAccessorDeclarations(io::Printer* printer) {
                Variables_["type"] = FieldMessageTypeName(Field_);

                printer->Print(Variables_,
                    "inline const $type$& Get$rname$() const { return $name$(); }\n"
                    "inline $type$* Mutable$rname$() { return mutable_$name$(); }\n");
                if (Variables_.end() != Variables_.find("RName"))
                    printer->Print(Variables_,
                        "inline const $type$& Get$RName$() const { return $name$(); }\n"
                        "inline $type$* Mutable$RName$() { return mutable_$name$(); }\n");
            }
    };

    class TRepeatedMessageFieldExtGenerator: public TFieldExtGenerator  {
        public:
            TRepeatedMessageFieldExtGenerator(const FieldDescriptor* field)
                : TFieldExtGenerator(field)
            {
            }

            void GenerateAccessorDeclarations(io::Printer* printer) {
                Variables_["type"] = FieldMessageTypeName(Field_);

                printer->Print(Variables_,
                    "inline const $type$& Get$rname$(int index) const { return $name$(index); }\n"
                    "inline $type$* Mutable$rname$(int index) { return mutable_$name$(index); }\n"
                    "inline $type$* Add$rname$() { return add_$name$(); }\n"
                    "inline const $type$& get_idx_$name$(int index) const { return $name$(index); }\n"
                    "inline const ::google::protobuf::RepeatedPtrField< $type$ >&\n"
                    "    get_arr_$name$() const { return $name$(); }\n"
                    "inline const ::google::protobuf::RepeatedPtrField< $type$ >&\n"
                    "    Get$rname$() const { return $name$(); }\n"
                    "inline ::google::protobuf::RepeatedPtrField< $type$ >*\n"
                    "    Mutable$rname$() { return mutable_$name$(); }\n");

                if (Variables_.end() != Variables_.find("RName"))
                    printer->Print(Variables_,
                        "inline const $type$& Get$RName$(int index) const { return $name$(index); }\n"
                        "inline $type$* Mutable$RName$(int index) { return mutable_$name$(index); }\n"
                        "inline $type$* Add$RName$() { return add_$name$(); }\n"
                        "inline const ::google::protobuf::RepeatedPtrField< $type$ >&\n"
                        "    Get$RName$() const { return $name$(); }\n"
                        "inline ::google::protobuf::RepeatedPtrField< $type$ >*\n"
                        "    Mutable$RName$() { return mutable_$name$(); }\n"
                    );
            }
    };

    class TStringFieldExtGenerator: public TFieldExtGenerator  {
        public:
            TStringFieldExtGenerator(const FieldDescriptor* field)
                : TFieldExtGenerator(field)
            {
            }

            void GenerateAccessorDeclarations(io::Printer* printer) {
                Variables_["pointer_type"] = Field_->type() == FieldDescriptor::TYPE_BYTES ? "void" : "char";

                if (Field_->options().ctype() != FieldOptions::STRING) {
                    printer->Outdent();
                    printer->Print(
                              " private:\n"
                              "  // Hidden due to unknown ctype option.\n");
                    printer->Indent();
                }

                printer->Print(Variables_,
                    "inline const TProtoStringType& Get$rname$() const { return $name$(); }\n"
                    "inline void Set$rname$(const TProtoStringType& value) { set_$name$(value); }\n"
                    "inline void Set$rname$(const char* value) { set_$name$(value); }\n"
                    "inline void Set$rname$(const $pointer_type$* value, size_t size) { set_$name$(value, size); }\n"
                    "inline TProtoStringType* Mutable$rname$() { return mutable_$name$(); }\n");

                if (Variables_.end() != Variables_.find("RName"))
                    printer->Print(Variables_,
                        "inline const TProtoStringType& Get$RName$() const { return $name$(); }\n"
                        "inline void Set$RName$(const TProtoStringType& value) { set_$name$(value); }\n"
                        "inline void Set$RName$(const char* value) { set_$name$(value); }\n"
                        "inline void Set$RName$(const $pointer_type$* value, size_t size) { set_$name$(value, size); }\n"
                        "inline TProtoStringType* Mutable$RName$() { return mutable_$name$(); }\n"
                    );

                if (Field_->options().ctype() != FieldOptions::STRING) {
                    printer->Outdent();
                    printer->Print(" public:\n");
                    printer->Indent();
                }

            }
    };

    class TRepeatedStringFieldExtGenerator: public TFieldExtGenerator  {
        public:
            TRepeatedStringFieldExtGenerator(const FieldDescriptor* field)
                : TFieldExtGenerator(field)
            {
            }

            void GenerateAccessorDeclarations(io::Printer* printer) {
                Variables_["pointer_type"] = Field_->type() == FieldDescriptor::TYPE_BYTES ? "void" : "char";

                if (Field_->options().ctype() != FieldOptions::STRING) {
                    printer->Outdent();
                    printer->Print(
                              " private:\n"
                              "  // Hidden due to unknown ctype option.\n");
                    printer->Indent();
                }

                printer->Print(Variables_,
                    "inline const TProtoStringType& Get$rname$(int index) const { return $name$(index); }\n"
                    "inline TProtoStringType* Mutable$rname$(int index) { return mutable_$name$(index); }\n"
                    "inline void Set$rname$(int index, const TProtoStringType& value) { set_$name$(index, value); }\n"
                    "inline void Set$rname$(int index, const char* value) { set_$name$(index, value); }\n"
                    "inline void Set$rname$(int index, const $pointer_type$* value, size_t size) { set_$name$(index, value, size); }\n"
                    "inline TProtoStringType* Add$rname$() { return add_$name$(); }\n"
                    "inline void Add$rname$(const TProtoStringType& value) { add_$name$(value); }\n"
                    "inline void Add$rname$(const char* value) { add_$name$(value); }\n"
                    "inline void Add$rname$(const $pointer_type$* value, size_t size) { add_$name$(value, size); }\n"
                    "inline const TProtoStringType& get_idx_$name$(int index) const { return $name$(index); }\n"
                    "inline const ::google::protobuf::RepeatedPtrField<TProtoStringType>& get_arr_$name$() const"
                    "{ return $name$(); }\n"
                    "inline const ::google::protobuf::RepeatedPtrField<TProtoStringType>& Get$rname$() const"
                    "{ return $name$(); }\n"
                    "inline ::google::protobuf::RepeatedPtrField<TProtoStringType>* Mutable$rname$()"
                    "{ return mutable_$name$(); }\n");

                if (Variables_.end() != Variables_.find("RName"))
                    printer->Print(Variables_,
                        "inline const TProtoStringType& Get$RName$(int index) const { return $name$(index); }\n"
                        "inline TProtoStringType* Mutable$RName$(int index) { return mutable_$name$(index); }\n"
                        "inline void Set$RName$(int index, const TProtoStringType& value) { set_$name$(index, value); }\n"
                        "inline void Set$RName$(int index, const char* value) { set_$name$(index, value); }\n"
                        "inline void Set$RName$(int index, const $pointer_type$* value, size_t size) { set_$name$(index, value, size); }\n"
                        "inline TProtoStringType* Add$RName$() { return add_$name$(); }\n"
                        "inline void Add$RName$(const TProtoStringType& value) { add_$name$(value); }\n"
                        "inline void Add$RName$(const char* value) { add_$name$(value); }\n"
                        "inline void Add$RName$(const $pointer_type$* value, size_t size) { add_$name$(value, size); }\n"
                        "inline const ::google::protobuf::RepeatedPtrField<TProtoStringType>& Get$RName$() const"
                        "{ return $name$(); }\n"
                        "inline ::google::protobuf::RepeatedPtrField<TProtoStringType>* Mutable$RName$()"
                        "{ return mutable_$name$(); }\n"
                    );

                if (Field_->options().ctype() != FieldOptions::STRING) {
                    printer->Outdent();
                    printer->Print(" public:\n");
                    printer->Indent();
                }
            }
    };

    class TEnumFieldExtGenerator: public TFieldExtGenerator  {
        public:
            TEnumFieldExtGenerator(const FieldDescriptor* field)
                : TFieldExtGenerator(field)
            {
            }

            void GenerateAccessorDeclarations(io::Printer* printer) {
                Variables_["type"] = ClassName(Field_->enum_type(), true);

                printer->Print(Variables_,
                    "inline $type$ Get$rname$() const { return $name$(); } \n"
                    "inline void Set$rname$($type$ value) { set_$name$(value); }\n");

                if (Variables_.end() != Variables_.find("RName"))
                    printer->Print(Variables_,
                        "inline $type$ Get$RName$() const { return $name$(); } \n"
                        "inline void Set$RName$($type$ value) { set_$name$(value); }\n"
                    );
            }
    };

    class TRepeatedEnumFieldExtGenerator: public TFieldExtGenerator  {
        public:
            TRepeatedEnumFieldExtGenerator(const FieldDescriptor* field)
                : TFieldExtGenerator(field)
            {
            }

            void GenerateAccessorDeclarations(io::Printer* printer) {
                Variables_["type"] = ClassName(Field_->enum_type(), true);

                printer->Print(Variables_,
                    "inline $type$ Get$rname$(int index) const { return $name$(index); }\n"
                    "inline void Set$rname$(int index, $type$ value) { set_$name$(index, value); }\n"
                    "inline void Add$rname$($type$ value) { add_$name$(value); }\n"
                    "inline $type$ get_idx_$name$(int index) const { return $name$(index); }\n"
                    "inline const ::google::protobuf::RepeatedField<int>& get_arr_$name$() const { return $name$(); }\n"
                    "inline const ::google::protobuf::RepeatedField<int>& Get$rname$() const { return $name$(); }\n"
                    "inline ::google::protobuf::RepeatedField<int>* Mutable$rname$() { return mutable_$name$(); }\n");
                if (Variables_.end() != Variables_.find("RName"))
                    printer->Print(Variables_,
                        "inline $type$ Get$RName$(int index) const { return $name$(index); }\n"
                        "inline void Set$RName$(int index, $type$ value) { set_$name$(index, value); }\n"
                        "inline void Add$RName$($type$ value) { add_$name$(value); }\n"
                        "inline const ::google::protobuf::RepeatedField<int>& Get$RName$() const { return $name$(); }\n"
                        "inline ::google::protobuf::RepeatedField<int>* Mutable$RName$() { return mutable_$name$(); }\n"
                    );
            }
    };

    class TPrimitiveFieldExtGenerator: public TFieldExtGenerator  {
        public:
            TPrimitiveFieldExtGenerator(const FieldDescriptor* field)
                : TFieldExtGenerator(field)
            {
            }

            void GenerateAccessorDeclarations(io::Printer* printer) {
                Variables_["type"] = PrimitiveTypeName(Field_->cpp_type());

                printer->Print(Variables_,
                    "inline $type$ Get$rname$() const { return $name$();}\n"
                    "inline void Set$rname$($type$ value) { set_$name$(value); }\n");
                if (Variables_.end() != Variables_.find("RName"))
                    printer->Print(Variables_,
                        "inline $type$ Get$RName$() const { return $name$();}\n"
                        "inline void Set$RName$($type$ value) { set_$name$(value); }\n"
                    );
            }
    };

    class TRepeatedPrimitiveFieldExtGenerator: public TFieldExtGenerator  {
        public:
            TRepeatedPrimitiveFieldExtGenerator(const FieldDescriptor* field)
                : TFieldExtGenerator(field)
            {
            }

            void GenerateAccessorDeclarations(io::Printer* printer) {
                Variables_["type"] = PrimitiveTypeName(Field_->cpp_type());

                printer->Print(Variables_,
                    "inline $type$ Get$rname$(int index) const { return $name$(index); }\n"
                    "inline void Set$rname$(int index, $type$ value) { set_$name$(index, value); }\n"
                    "inline void Add$rname$($type$ value) { add_$name$(value); }\n"
                    "inline $type$ get_idx_$name$(int index) const { return $name$(index); }\n"
                    "inline const ::google::protobuf::RepeatedField< $type$ >&\n"
                    "    get_arr_$name$() const { return $name$(); }\n"
                    "inline const ::google::protobuf::RepeatedField< $type$ >&\n"
                    "    Get$rname$() const { return $name$(); }\n"
                    "inline ::google::protobuf::RepeatedField< $type$ >*\n"
                    "    Mutable$rname$() { return mutable_$name$(); }\n");
                if (Variables_.end() != Variables_.find("RName"))
                    printer->Print(Variables_,
                        "inline $type$ Get$RName$(int index) const { return $name$(index); }\n"
                        "inline void Set$RName$(int index, $type$ value) { set_$name$(index, value); }\n"
                        "inline void Add$RName$($type$ value) { add_$name$(value); }\n"
                        "inline const ::google::protobuf::RepeatedField< $type$ >&\n"
                        "    Get$RName$() const { return $name$(); }\n"
                        "inline ::google::protobuf::RepeatedField< $type$ >*\n"
                        "    Mutable$RName$() { return mutable_$name$(); }\n"
                    );
            }
    };

    // borrowed mostly from protobuf/compiler/cpp/cpp_extension.cc
    class TExtensionGenerator {
    public:
        TExtensionGenerator(const FieldDescriptor* descriptor)
            : Descriptor_(descriptor)
        {
            if (Descriptor_->is_repeated()) {
                type_traits_ = "Repeated";
            }

            TProtoStringType clsName;
            switch (Descriptor_->cpp_type()) {
            case FieldDescriptor::CPPTYPE_ENUM:
                type_traits_.append("EnumTypeTraits< ");
                clsName = ClassName(Descriptor_->enum_type(), true);
                type_traits_.append(clsName);
                type_traits_.append(", ");
                type_traits_.append(clsName);
                type_traits_.append("_IsValid>");
                break;
            case FieldDescriptor::CPPTYPE_STRING:
                type_traits_.append("StringTypeTraits");
                break;
            case FieldDescriptor::CPPTYPE_MESSAGE:
                type_traits_.append("MessageTypeTraits< ");
                type_traits_.append(ClassName(Descriptor_->message_type(), true));
                type_traits_.append(" >");
                break;
            default:
                type_traits_.append("PrimitiveTypeTraits< ");
                type_traits_.append(PrimitiveTypeName(Descriptor_->cpp_type()));
                type_traits_.append(" >");
                break;
            }
        }

        void GenerateDeclaration(io::Printer* printer) const
        {
            TVariables vars;
            vars["extendee"     ] = ClassName(Descriptor_->containing_type(), true);
            vars["type_traits"  ] = type_traits_;
            vars["name"         ] = Descriptor_->name();
            vars["field_type"   ] = SimpleItoa(static_cast<int>(Descriptor_->type()));
            vars["packed"       ] = Descriptor_->options().packed() ? "true" : "false";

            printer->Print(vars,
              "typedef ::google::protobuf::internal::ExtensionIdentifier< $extendee$,\n"
              "    ::google::protobuf::internal::$type_traits$, $field_type$, $packed$ >\n"
              "  Td$name$;\n"
              );
        }

    private:
        const FieldDescriptor* Descriptor_;
        TProtoStringType type_traits_;
    };

    TFieldExtGenerator* MakeGenerator(const FieldDescriptor* field) {
        if (field->is_repeated()) {
            switch (field->cpp_type()) {
                case FieldDescriptor::CPPTYPE_MESSAGE:
                    return new TRepeatedMessageFieldExtGenerator(field);
                case FieldDescriptor::CPPTYPE_STRING:
                    switch (field->options().ctype()) {
                        default:  // RepeatedStringFieldExtGenerator handles unknown ctypes.
                        case FieldOptions::STRING:
                            return new TRepeatedStringFieldExtGenerator(field);
                    }
                case FieldDescriptor::CPPTYPE_ENUM:
                    return new TRepeatedEnumFieldExtGenerator(field);
                default:
                    return new TRepeatedPrimitiveFieldExtGenerator(field);
            }
        } else {
            switch (field->cpp_type()) {
                case FieldDescriptor::CPPTYPE_MESSAGE:
                    return new TMessageFieldExtGenerator(field);
                case FieldDescriptor::CPPTYPE_STRING:
                    switch (field->options().ctype()) {
                        default:  // StringFieldGenerator handles unknown ctypes.
                        case FieldOptions::STRING:
                            return new TStringFieldExtGenerator(field);
                    }
                case FieldDescriptor::CPPTYPE_ENUM:
                    return new TEnumFieldExtGenerator(field);
                default:
                    return new TPrimitiveFieldExtGenerator(field);
            }
        }
    }

    class TMessageExtGenerator {
        public:
            TMessageExtGenerator(const Descriptor* descriptor, OutputDirectory* outputDirectory)
                : Descriptor_(descriptor)
                , Classname_(ClassName(descriptor, false))
                , OutputDirectory_(outputDirectory)
                , FieldGenerators_(new scoped_ptr<TFieldExtGenerator>[descriptor->field_count()])
                , NestedGenerators_(new scoped_ptr<TMessageExtGenerator>[descriptor->nested_type_count()])
                , ExtensionGenerators_(new scoped_ptr<TExtensionGenerator>[descriptor->extension_count()])
            {
                for (int i = 0; i < descriptor->nested_type_count(); i++) {
                    NestedGenerators_[i].reset(new TMessageExtGenerator(descriptor->nested_type(i), OutputDirectory_));
                }

                for (int i = 0; i < descriptor->field_count(); i++) {
                    FieldGenerators_[i].reset(MakeGenerator(descriptor->field(i)));
                }

                for (int i = 0; i < descriptor->extension_count(); i++) {
                    ExtensionGenerators_[i].reset(new TExtensionGenerator(descriptor->extension(i)));
                }
            }

            void GenerateClassDefinitionExtension() {
                GenerateFieldAccessorDeclarations();

                for (int i = 0; i < Descriptor_->nested_type_count(); i++) {
                    NestedGenerators_[i]->GenerateClassDefinitionExtension();
                }
            }

            void GenerateClassExtension() {
            }

        private:
            void GenerateFieldAccessorDeclarations() {
                TProtoStringType fileName = HeaderFileName(Descriptor_->file());
                TProtoStringType scope = "class_scope:" + Descriptor_->full_name();
                scoped_ptr<io::ZeroCopyOutputStream> output(
                    OutputDirectory_->OpenForInsert(fileName, scope));
                io::Printer printer(output.get(), '$');

                printer.Print("// Yandex cpp-styleguide extension\n");
                for (int i = 0; i < Descriptor_->field_count(); i++) {
                    const FieldDescriptor* field = Descriptor_->field(i);

                    TVariables vars;
                    SetCommonFieldVariables(field, &vars);

                    const bool hasRName = (vars.end() != vars.find("RName"));
                    if (field->is_repeated()) {
                        printer.Print(vars,
                            "inline size_t $rname$Size() const { return (size_t)$name$_size(); }\n");
                        if (hasRName)
                            printer.Print(vars,
                                "inline size_t $RName$Size() const { return (size_t)$name$_size(); }\n");
                    } else {
                        printer.Print(vars,
                            "inline bool Has$rname$() const { return has_$name$(); }\n");
                        if (hasRName)
                            printer.Print(vars,
                                "inline bool Has$RName$() const { return has_$name$(); }\n");
                    }

                    printer.Print(vars, "inline void Clear$rname$() { clear_$name$(); }\n");
                    if (hasRName)
                        printer.Print(vars,
                            "inline void Clear$RName$() { clear_$name$(); }\n");

                    // Generate type-specific accessor declarations.
                    FieldGenerators_[i]->GenerateAccessorDeclarations(&printer);

                    printer.Print("\n");
                }
                for (int i = 0; i < Descriptor_->extension_count(); i++) {
                    ExtensionGenerators_[i]->GenerateDeclaration(&printer);
                }
                printer.Print("// End of Yandex-specific extension\n");
            }

        private:
            const Descriptor* Descriptor_;
            const TProtoStringType Classname_;
            OutputDirectory* OutputDirectory_;
            scoped_array<scoped_ptr<TFieldExtGenerator> >  FieldGenerators_;
            scoped_array<scoped_ptr<TMessageExtGenerator> > NestedGenerators_;
            scoped_array<scoped_ptr<TExtensionGenerator> > ExtensionGenerators_;
    };

    class TFileExtGenerator {
        public:
            TFileExtGenerator(const FileDescriptor* file, OutputDirectory* output_directory)
                : OutputDirectory_(output_directory)
                , MessageTypeCount_(file->message_type_count())
                , MessageGenerators_(new scoped_ptr<TMessageExtGenerator>[MessageTypeCount_])
            {
                for (int i = 0; i < MessageTypeCount_; i++) {
                    MessageGenerators_[i].reset(new TMessageExtGenerator(file->message_type(i), OutputDirectory_));
                }
            }

            void GenerateHeaderExtensions() {
                for (int i = 0; i < MessageTypeCount_; i++) {
                    MessageGenerators_[i]->GenerateClassDefinitionExtension();
                }

            }

            void GenerateSourceExtensions() {
                {
                    //TProtoStringType fileName = SourceFileName(File_);
                    //scoped_ptr<io::ZeroCopyOutputStream> output(
                    //    output_directory_->OpenForInsert(fileName, "namespace_scope"));
                    //io::Printer printer(output.get(), '$');
                }

                for (int i = 0; i < MessageTypeCount_; i++) {
                    MessageGenerators_[i]->GenerateClassExtension();
                }
            }

        private:
            OutputDirectory* OutputDirectory_;
            size_t MessageTypeCount_;
            scoped_array<scoped_ptr<TMessageExtGenerator> > MessageGenerators_;
    };

    bool TCppStyleGuideExtensionGenerator::Generate(const FileDescriptor* file,
        const TProtoStringType& parameter,
        OutputDirectory* outputDirectory,
        TProtoStringType* error) const {

        TFileExtGenerator fileGenerator(file, outputDirectory);

        // Generate header.
        fileGenerator.GenerateHeaderExtensions();

        // Generate cc file.
        //fileGenerator.GenerateSourceExtensions();

        return true;
    }

}
}
}

int main(int argc, char* argv[]) {
    #ifdef _MSC_VER
    // Don't print a silly message or stick a modal dialog box in my face,
    // please.
    _set_abort_behavior(0, ~0);
#endif  // !_MSC_VER

    NProtobuf::NCompiler::NPlugins::TCppStyleGuideExtensionGenerator generator;
    return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
