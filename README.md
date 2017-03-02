# OGCCPlayer #

Odtwaracz plików muzycznych stworzyny przy pomocy Qt 4.8 i biblioteki bass.

![screen1.png](img/screen1.png)

## Biblioteka bass

[Bass](http://www.un4seen.com/)

### Instalacja Linux
1. Przejdź do katalogu bass
1. skopiuj bass.h do /usr/local/include/
2. stwórz katalog /usr/local/lib/bass24
3. skopiuj libbass.so do /usr/local/lib/bass24

## FAQ

1. cannot find -lGL

Doinstaluj libgl1-mesa-dev
`sudo apt-get install libgl1-mesa-dev`