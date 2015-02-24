#encoding "utf-8"

PersonName -> Word<kwtype="имя">;

Person -> PersonName interp (Person.Name);
