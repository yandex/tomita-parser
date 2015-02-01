#encoding "utf-8"    // сообщаем парсеру о том, в какой кодировке написана грамматика
#GRAMMAR_ROOT S      // указываем корневой нетерминал грамматики

ProperName ->  Word<h-reg1>+;                       // задание имени собственного
Person -> ProperName | 'человек';                   // человек может обозначаться именем собственным
                                                    // или словом “человек”
FormOfAddress -> 'товарищ' | 'мистер' | 'господин'; // перечисление возможных форм обращения

AdjCoord -> Adj;
AdjCoord -> AdjCoord<gnc-agr[1]> ',' Adj<gnc-agr[1]>;
AdjCoord -> AdjCoord<gnc-agr[1]> 'и' Adj<gnc-agr[1]>;

S -> Adj<gnc-agr[1]>+ (FormOfAddress) Person<gnc-agr[1], rt>; // для случаев, когда прилагательные
                                                              // идут друг за другом
S -> AdjCoord<gnc-agr[1]> (FormOfAddress) Person<gnc-agr[1], rt>; // для случаев, когда прилагательные
                                                              // разделены запятой или сочинительным союзом
