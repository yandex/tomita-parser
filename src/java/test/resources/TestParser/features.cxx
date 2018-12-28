#encoding "utf8"

#GRAMMAR_ROOT Features

WarmFloor -> "полы" | "пол";
FloorCoveringOrdinar -> "ламинат" | "линолеум" | "линолиум" | "ковролин";
FloorCoveringExcellent -> "паркет" | "доска";
TileFinish -> "плитка" | "кафель" | "плитке" | "плиткой" | "кафелем";
Equipment -> "техника" | "гарнитур" | "мебель" | "гардероб" | "шкаф-купе";
Ceiling -> "потолки" | "потолок";
Conditioners -> "кондиционер" | "сплит-система" | "сплитсистема";
//Windows -> "окна" | "стеклопакеты";
Sanitary -> "сантехника";
Materials -> "материал";
//MetalDoor -> "двери";
Tie -> "стяжка";
Plaster -> "штукатурка" | "оштукатуренные" | "оштукатурены" | "отштукатурены";
//Radiators -> "радиаторы";
Wallpaper -> "обои";

Quality -> "дорогой" | "качественный" | "дорогостоящий" | "европейский" | "импортный" | "новый" | "современный" | "свежий" | "итальянский" | "немецкий" | "испанский" | "элитный";
//Metall -> "железная" | "металлическая";
//Plastik -> "пластиковые" | "ПВХ" | "пластик" | "евро";
//Interior -> "межкомнатные";
Tension -> "натяжные" | "натяжной";
//Integrated -> "встроенная";
Warm -> "теплые" | "теплый";
//RadiatorsQuality -> "новые" | "биметаллические" | "аллюминиевые" | "заменены" | "поменяны";

Features -> Warm<gnc-agr[1]> WarmFloor<gnc-agr[1]> interp (Features.WarmFloor);

Features -> FloorCoveringOrdinar interp (Features.FloorCoveringOrdinar);

Features -> FloorCoveringExcellent interp (Features.FloorCoveringExcellent);

Features -> TileFinish interp (Features.TileFinish);

Features -> Equipment interp (Features.Equipment);

Features -> Tension interp (Features.Ceiling) Ceiling;

Features -> Ceiling Tension interp (Features.Ceiling);

Features -> Conditioners interp (Features.Conditioners);

//Features -> Plastik interp (Features.Windows);

Features -> Quality<gnc-agr[1]> Sanitary<gnc-agr[1]> interp (Features.Sanitary);

Features -> Quality<gnc-agr[1]> Materials<gnc-agr[1]> interp (Features.Materials);

//Features -> Interior MetalDoor interp (Features.MetalDoor);

Features -> Tie interp (Features.Tie);

Features -> Plaster interp (Features.Plaster);

//Features -> Radiators interp (Features.Radiators);

Features -> Quality<gnc-agr[1]> Wallpaper<gnc-agr[1]> interp (Features.Wallpaper);
