#encoding "utf8"

#GRAMMAR_ROOT Label

RepairLabel -> "capital" | "cosmetic" | "good" | "excellent";

Label -> RepairLabel interp (Labels.Label);
