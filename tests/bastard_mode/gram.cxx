#encoding "utf-8"

NP -> Adj<gnc-agr[1]> NP<gnc-agr[1], rt>;
NP -> Noun;

VP -> Adv VP<rt>;
VP -> Verb;

S -> NP interp(SVO.S) 
     VP interp(SVO.V)
     NP interp(SVO.O);
