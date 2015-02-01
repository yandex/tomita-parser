#encoding "utf-8"    // сообщаем парсеру о том, в какой кодировке написана грамматика

ProperName -> Word<h-reg1, gram='имя'>
              Word<h-reg1, gram='отч'>
              Word<h-reg1, gram='фам'>;

S -> ProperName 'родиться'<gram='прош'> 'в'
     AnyWord<wff=/[1-2]?[0-9]{1,3}г?\.?/> ('год' <gram='ед, дат'>);
