#encoding "utf8"

#GRAMMAR_ROOT Label

//RepairLabel -> "capital" | "cosmetic" | "good" | "excellent";
RepairLabel -> "documentid";

Label -> RepairLabel AnyWord<wfm="[0-9].*"> interp (Labels.Label);
