**Вопросы:**

1. Коротко опишите, что делают флаги утилиты `ar`: `r`, `c`, `s` . Что произойдёт, если запустить команду второй раз с тем же именем архива?

- `r` - добавить файлы в архив, а если файл с таким именем уже есть, заменить его
- `c` - создать архив, если его нет, без предупреждения
- `s` - записать в архив индекс символов (как `ranlib`), чтобы компоновщик быстро находил нужные функции. При повторном запуске архив не пересоздаётся, а `vector_math.o` внутри просто заменяется новой версией, дублей не появляется

```bash
$ gcc -c vector_math.c -o vector_math.o
$ ar rcs libmath_static.a vector_math.o
$ g++ -c main.cpp -o main.o
$ g++ main.o -L. -lmath_static -o app_static
$ ./app_static
Library version: v2.4.1-release
Dot product result: 32
```

2. С помощью утилиты `nm` посмотрите, что получилось в `main.o`. Как вы думаете, почему функция `main` не подверглась name mangling (имеет "чистое" имя), хотя файл скомпилирован компилятором C++?

```bash
$ nm main.o
                 U _GLOBAL_OFFSET_TABLE_
                 U _ZNSolsEPFRSoS_E
                 U _ZNSolsEi
0000000000000027 r _ZNSt8__detail30__integer_to_chars_is_unsignedIjEE
0000000000000028 r _ZNSt8__detail30__integer_to_chars_is_unsignedImEE
0000000000000029 r _ZNSt8__detail30__integer_to_chars_is_unsignedIyEE
                 U _ZSt21ios_base_library_initv
                 U _ZSt4cout
                 U _ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_
                 U _ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc
                 U __stack_chk_fail
                 U dot_product
                 U get_lib_version
0000000000000000 T main
```

`main` вызывает стартовый код C (`crt`) и он ищет символ именно с именем `main`. Перегружать `main` стандарт C++ запрещает, поэтому компилятор не декорирует его имя. `dot_product` и `get_lib_version` тоже без декорирования, потому что объявлены внутри `extern "C"`

3. Приведите текст полученной на шаге 1 ошибки. *Комментарий: для создания `.a` флаг `-fPIC` не нужен, а для `.so` — обязателен. Подробное объяснение механизма, стоящего за этим ограничением — таблицы GOT и позиционно-независимый код, — будет дано на четвёртой лекции.*

Командами из задания ошибка не воспроизводится, библиотека собирается:

```bash
$ gcc -c vector_math.c -o vector_math_nopic.o
$ gcc -shared -o libmath_broken.so vector_math_nopic.o
$ echo $?
0
```

В современных Ubuntu/Debian `gcc` по умолчанию собирает с `-fPIE` (`--enable-default-pie`), поэтому код и без флага получается позиционно-независимым. Если явно отключить PIC, ошибка появляется:

```bash
$ gcc -c -fno-pic -O2 vector_math.c -o vector_math_nopic.o
$ gcc -shared -o libmath_broken.so vector_math_nopic.o
/usr/bin/ld: vector_math_nopic.o: relocation R_X86_64_32 against `.rodata.str1.1' can not be used when making a shared object; recompile with -fPIC
/usr/bin/ld: failed to set dynamic section sizes: bad value
collect2: error: ld returned 1 exit status
```

4. Сравните размеры сегментов `text` для `app_static` и `app_dynamic` (через `size`). Куда делся код функций библиотеки в динамическом варианте?

```bash
$ size app_static app_dynamic
   text    data     bss     dec     hex filename
   2548     656     280    3484     d9c app_static
   2517     680     280    3477     d95 app_dynamic
```

В `app_dynamic` `text` меньше на 31 байт. Разница маленькая, потому что сами функции маленькие, а в динамический вариант добавляются заглушки PLT для их вызова

Код функций лежит в `libmath_dynamic.so` и подгружается при запуске программы, в самом `app_dynamic` они только `U`:

```bash
$ nm app_static | grep -E 'dot_product|get_lib'
00000000000012ad T dot_product
000000000000129c T get_lib_version
$ nm app_dynamic | grep -E 'dot_product|get_lib'
                 U dot_product
                 U get_lib_version
```

Поэтому для запуска нужно указать, где искать библиотеку: `LD_LIBRARY_PATH=. ./app_dynamic`

5. Шаг 1 упадёт с ошибкой `undefined reference`. Объясните, почему это происходит, используя концепцию множеств компоновщика: `U` (неразрешённые ссылки), `D` (определённые символы) и `E` (извлечённые файлы).

```bash
$ g++ -L. -lmath_dynamic main.o -o app_wrong_order
/usr/bin/ld: main.o: in function `main':
main.cpp:(.text+0x63): undefined reference to `get_lib_version'
/usr/bin/ld: main.cpp:(.text+0xb4): undefined reference to `dot_product'
collect2: error: ld returned 1 exit status
```

Компоновщик идёт по аргументам слева направо и один раз. Когда он доходит до библиотеки, `U` ещё пустое, потому что `main.o` он ещё не видел

Библиотека не разрешает ни одной нужной ссылки, поэтому в `E` из неё ничего не попадает. Потом обрабатывается `main.o`: `main` идёт в `D`, а `dot_product` и `get_lib_version` - в `U`

К библиотеке компоновщик уже не возвращается, и в конце `U` не пустое, отсюда `undefined reference`

6. Линковка упадёт с ошибкой `undefined reference`. Выполните `nm main.o | grep dot` и `nm libmath_dynamic.so | grep dot`. Как теперь выглядят имена функций `dot_product` и `get_lib_version` в объектном файле C++?

```bash
$ nm main.o | grep dot
                 U _Z11dot_productPKiS0_i
$ nm libmath_dynamic.so | grep dot
000000000000110a T dot_product
$ nm main.o | grep lib_version
                 U _Z15get_lib_versionv
$ nm libmath_dynamic.so | grep lib_version
00000000000010f9 T get_lib_version
```

Без `extern "C"` компилятор C++ декорирует имена: `_Z11dot_productPKiS0_i` и `_Z15get_lib_versionv`. А в библиотеке, собранной компилятором C, имена обычные: `dot_product` и `get_lib_version`. Имена не совпадают, поэтому компоновщик не находит функции
