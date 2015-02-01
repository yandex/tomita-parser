#encoding "utf-8"
#include <date.cxx>

Animal -> Noun<kwtype=animal>;
WithWho -> 'у' (Adj<gnc-agr[1]>) Animal<gram='род', rt, gnc-agr[1]> interp (Sparrow.Who);
WithWho -> 'с' (Adj<gnc-agr[1]>) Animal<gram='твор', rt, gnc-agr[1]> interp (Sparrow.Who);
S -> Date interp (Sparrow.When) WithWho;
S -> WithWho Date interp (Sparrow.When);
