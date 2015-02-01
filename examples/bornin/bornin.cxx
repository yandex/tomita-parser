#encoding "utf-8"

Born -> Verb<kwtype=born>;
City -> Noun<kwtype=city>;
//Person -> AnyWord<gram="имя">;

//S -> Noun interp(BornFact.Person) Born "в" Noun interp(BornFact.Place);
S -> AnyWord<gram="имя"> interp(BornFact.Person) Born "в" City interp(BornFact.Place);

