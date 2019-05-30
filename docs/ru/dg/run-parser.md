# Запуск парсера и файл конфигурации

Имя конфигурационного файла — единственный аргумент программы tomitaparser. Т.е. запускать надо вот так:

В Linux, FreeBSD и прочих *nix системах:

```no-highlight
./tomitaparser config.proto
```

В Windows:

```
tomitaparser.exe config.proto
```

В конфигурационном файле указывается:

- где находятся тексты, которые нужно обработать;
- путь к корневому словарю;
- какие статьи корневого словаря запускать;
- какие факты нужно сохранить и в каком формате;
- куда сохранять факты;
- несколько вспомогательных параметров.



## Описание формата конфигурационного файла <a name="opisanieformatakonfiguracionnogofajjla"></a>

```no-highlight
message TTextMinerConfig {
  message TInputParams {
    enum SourceFormat {
      plain = 0;
      html = 1;
    }
    enum SourceType {
      no = 0;
      dpl = 1;
      mapreduce = 2;
      arcview = 3;
      tar = 4;
      som = 5;
      yarchive = 6;
    }
    optional string File = 1 [default = ""];            // Если File пропущено — то STDIN
    optional string Dir = 2 [default = ""];             // Если читаем файлы из папки
    optional SourceFormat Format = 3 [default = plain]; // Если Format пропущено — то plain

    optional SourceType Type = 4;                       // Если Type пропущено — то читаем обычный файл (то же, что "no")
    optional string Encoding = 5 [default = "utf8"];    // Кодировка входных данных (если не задано — utf8)
    optional string SubKey = 6;                         // Для mapreduce
    optional int32 FirstDocNum = 7 [default = -1];      // Для arcview: первый документ, который надо анализировать
                                                        // (если не задано — с начала архива)
    optional int32 LastDocDum = 8 [default = -1];       // Для arcview: последний документ, который надо анализировать
                                                        // (если не задано — до конца архива)
    optional string Url2Fio = 9;                        // Таблица url2fio для interview
    optional string Date = 10;                          // Дата для форматов кроме arcview
  }
  message TOutputParams {
    enum OutputMode {
      append = 0;
      overwrite = 1;
    }
    enum OutputFormat {
      xml = 0;
      text = 1;
      proto = 2;
      mapreduce = 3;
    }
    optional string File = 1 [default = ""];            // Если не задано — то STDOUT
    optional OutputMode Mode = 2 [default = append];    // Если не задано явно, но файл существует — ошибка
    optional OutputFormat Format = 3 [default = xml];   // Если не задано — то xml
    optional string Encoding = 4 [default = "utf8"];    // Кодировка выходных данных, по умолчанию — utf8
    optional bool SaveLeads = 5 [default = false];      // Сохранять ли лиды
    optional bool SaveEquals = 6 [default = false];     // Сохранять ли equals facts
  }
  message TArticleRef {
    required string Name = 1;
  }
  message TFactTypeRef {
    required string Name = 1;
    optional bool NonEquality = 2 [default = false];
  }
  required string Dictionary = 1;                       // Может быть папкой, тогда парсер будет работать
                                                        // в режиме совместимости со старыми путями
  optional string PrettyOutput = 2;                     // Имя файла, куда будет писаться отладочный HTML файл
  optional uint32 NumThreads = 3 [default = 1];         // кол-во потоков (по умолчанию — один)
  optional string Language = 4 [default = "ru"];        // Язык (по умолчанию — русский)
  optional string PrintTree = 5;                        // см. parsedoc
  optional string PrintRules = 6;                       // см. parsedoc
  optional TInputParams Input = 7;
  optional TOutputParams Output = 8;
  repeated TArticleRef Articles = 9;
  repeated TFactTypeRef Facts = 10;
  optional bool SavePartialFacts = 11 [default = true];                                         
  repeated TArticleRef ArticlesBeforeFragmentation = 12;
}
```


## Отладочный вывод <a name="otladochnyjjvyvod"></a>

Парсер может вывести на экран или сохранить в файл подробную информацию о ходе анализа текста. Формат вывода этой отладочной информации предназначен для удобства просмотра и не предназначен для автоматической обработки. Параметр `PrintTree` указывает файл, в который будет записаны деревья синтаксического разбора. Параметр `PrintRules` — применяемые правила. В качестве имени файла можно указать `STDOUT` или `STDERR`, чтобы вывести отладочную информацию в соответствующий поток.

Параметры `PrintTree` и `PrintRules` можно использовать одновременно.


### Деревья синтаксического разбора (параметр PrintTree) <a name="derevjasintaksicheskogorazboraparametrprinttree"></a>

Параметр `PrintTree` позволяет указать файл, в который будут выведены синтаксические деревья, которые парсер строит в процессе разбора.

**Пример**

```no-highlight
// грамматика
S -> Adj Noun;
// текст
Тяжелый труд облагораживает хорошего человека.
// фрагмент конфигурационного файла
PrintTree="tree.txt";
```

В результате работы парсера будет сформирован файл `tree.txt`, в который будут помещены все построенные парсером деревья.

**tree.txt**

```no-highlight
=====================first.cxx======================
coverage: 2, weight: 0.36666666 
S  ->  Adj[*Тяжелый]  Noun[труд] :: 0.43333333
|
|__ Adj  ->  Тяжелый :: 1
|
|__ Noun  ->  труд :: 1
coverage: 2, weight: 0.34
S  ->  Adj[*хорошего]  Noun[человека] :: 0.43333333
| 
|__ Adj  ->  хорошего :: 1
|
|__ Noun  ->  человека :: 1
======================multiwords======================
Тяжелый_труд: TAuxDicArticle, 
              наша_первая_грамматика,
хорошего_человека: TAuxDicArticle,
              наша_первая_грамматика,
```


### Срабатывание правил (параметр PrintRules) <a name="srabatyvaniepravilparametrprintrules"></a>

Параметр `PrintRules` позволяет указать файл, в который будет выведен список сработавших правил.

**Пример**

```no-highlight
// грамматика
S -> Adj Noun;
// текст
Тяжелый труд облагораживает хорошего человека.
// фрагмент конфигурационного файла
PrintRules="rules.txt";
```

**rules.txt**

```no-highlight
============================processing first.cxx=============================


S  ->  Adj[*Тяжелый]  Noun[труд] ::


S  ->  Adj[*хорошего]  Noun[человека] ::


****************************finished first.cxx*******************************
```


## Примеры <a name="primery"></a>

Важно: не забывать указывать кодировку самого конфига.

```no-highlight
encoding "utf8";
TTextMinerConfig {
  Dictionary = "../tomita-work/simple.gzt";
  Articles = [
    { Name = "_статья" }
  ]
  Facts = [
    { Name = "TFdo" }
  ]
}
```

Будет читать текст с консоли как один документ в кодировке utf8. Запустит статью <q>_статья</q> из корневого gzt-файла <q>../tomita-work/simple.gzt</q>. Напечатает XML с фактами типа TFdo в консоль.

```no-highlight
encoding "utf8";
TTextMinerConfig {
  Dictionary = "../tomita-work/simple.gzt";
  Input = {
    File = "fdo.txt";
    Encoding = "windows-1251";
  }
  Articles = [
    { Name = "_статья" }
  ]
  Facts = [
    { Name = "TFdo" }
  ]
}
```

Прочитает файл fdo.txt как один документ в кодировке Windows-1251 (надо явно указывать, т.к. по умолчанию — utf8). Запустит статью <q>_статья</q> из корневого gzt-файла <q>../tomita-work/simple.gzt</q>. Напечатает XML с фактами типа TFdo в консоль.

```no-highlight
encoding "utf8";
TTextMinerConfig {
  Dictionary = "../tomita-work/simple.gzt";
  Input = {
    File = "fdo.txt";
    Encoding = "windows-1251";
  }
  Output = {
    File = "facts.proto";
    Format = proto;
  }
  Articles = [
    { Name = "_статья" }
  ]
  Facts = [
    { Name = "TFdo" }
  ]
}
```

Сохранит файл с фактами типа TFdo в facts.proto.

