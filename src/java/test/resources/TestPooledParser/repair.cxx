#encoding "utf8"

#GRAMMAR_ROOT Repair

RepairW -> "ремонт" | "состояние" | "отделка" | "косметика" | "сост";

RepairEvro -> "евроремонт" | "евроремонтом" | "евроремонта" | "евроотделка";

RepairEvroSocr -> "евро";

VerbRepair -> "требует" | "требуется" | "нужен" | "нуждается";

Flat -> "квартира" | "Квартира" | "квартира-студия" | "студия";

With -> "с";

Without -> "без";

WithoutAfter -> "нет";

After -> "после" | "После";

From -> "от";

Complete -> "произведен" | "Произведен" | "был" | "выполнен" | "сделан";

For -> "под" | "Под";

Partly -> "частично";

Year -> "лет" | "года";

Ago -> "назад";

Design -> "дизайнерский" | "дизайн";

Project -> "проект";

//Repair -> (Verb<gnc-agr[1]> interp (Repair.Verb)) Adj<gnc-agr[1]>+ interp (Repair.AdjGroup) RepairW<gnc-agr[1]> interp (Repair.Word);
//Repair -> Verb<gnc-agr[1]> interp (Repair.Verb) RepairW<gnc-agr[1]> interp (Repair.Word);
//Repair -> RepairW<gnc-agr[1], h-reg1> interp (Repair.Word) Adj<gnc-agr[1], l-reg>+ interp (Repair.AdjGroup);

Repair -> Verb<gnc-agr[1]> interp (Repair.Verb) Adj<gnc-agr[1]>+ interp (Repair.AdjGroup) RepairW<gnc-agr[1]>;
Repair -> Verb interp (Repair.Verb) RepairW;
Repair -> RepairW<gnc-agr[1], h-reg1> Adj<gnc-agr[1], l-reg>+ interp (Repair.AdjGroup);
Repair -> RepairW<gnc-agr[1]> Adj<gnc-agr[1]> interp (Repair.AdjGroup);
Repair -> RepairW<gnc-agr[1]> Flat<gnc-agr[1]>  Adj<gnc-agr[1]> interp (Repair.AdjGroup);
Repair -> Adj<gnc-agr[1]>+ interp (Repair.AdjGroup) RepairW<gnc-agr[1]> Verb<gnc-agr[1]> interp (Repair.Verb);
Repair -> Adj+ interp (Repair.AdjGroup) RepairW;
Repair -> RepairW Verb interp (Repair.Verb);
Repair -> RepairEvroSocr interp (Repair.AdjGroup) RepairW;
Repair -> RepairEvro interp (Repair.Word);
Repair -> Prep<gnc-agr[1]> RepairW<gnc-agr[1]> interp (Repair.Word);
Repair -> VerbRepair<gnc-agr[1]> interp (Repair.Verb) RepairW<gnc-agr[1]>;
Repair -> RepairW VerbRepair interp (Repair.Verb);
Repair -> Without interp (Repair.Verb) RepairW;
Repair -> RepairW WithoutAfter interp (Repair.Verb);
Repair -> Flat With RepairW interp (Repair.Verb);
Repair -> Comma With RepairW interp (Repair.Verb);
Repair -> After RepairW interp (Repair.Verb);
Repair -> RepairW interp (Repair.Word) Complete;
Repair -> Complete RepairW interp (Repair.Word);
Repair -> For interp (Repair.Verb) RepairW;
Repair -> RepairW From Noun interp (Repair.AdjGroup);
Repair -> RepairW<h-reg1> interp (Repair.Word) Verb;
Repair -> Partly interp (Repair.AdjGroup) (With) RepairW;
Repair -> VerbRepair interp (Repair.Verb) Adj RepairW;
Repair -> Adj RepairW WithoutAfter interp (Repair.Verb);
Repair -> RepairEvro WithoutAfter interp (Repair.Verb);
Repair -> RepairW Verb AnyWord<wfl="[0-9]"> Year Ago interp (Repair.Verb);
Repair -> Design interp (Repair.AdjGroup) Project;
