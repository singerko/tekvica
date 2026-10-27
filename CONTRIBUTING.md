# Prispievanie

Ďakujeme za záujem o projekt. Zmeny posielajte cez samostatnú vetvu a GitLab merge request.

## Pred odoslaním zmeny

1. Popíšte použitý hardvér, rozmery a verziu dosky Pico.
2. Pri zmene mechaniky upravte najprv parametrický súbor v `scad/`; nový STL pridajte iba vtedy, keď je určený na tlač.
3. Pri novom STL uveďte počet kusov v názve a doplňte odkaz do `README.md`.
4. Pri zmene pinov aktualizujte zdrojový kód, schému aj dokumentáciu v jednom merge requeste.
5. Pri zmene firmvéru uveďte, ako bola zmena odskúšaná cez sériovú linku a na reálnom hardvéri.
6. Nepridávajte Arduino build výstupy, dočasné OpenSCAD súbory ani osobné nastavenia editora.

## Štýl zmien

- Zachovajte slovenské komentáre a názvy používateľských príkazov.
- Hardvérové limity a bezpečnostné kontroly neodstraňujte bez vysvetlenia.
- Fotografie ukladajte do `obrazky/`, schémy do `zapojenie/` a používajte opisné názvy.
- Veľké binárne súbory držte pod limitom GitLab repozitára; pri ďalšom raste zvážte Git LFS.

## Kontrola

Pred merge requestom overte:

- otvorenie oboch Arduino sketchov bez chýbajúcich lokálnych súborov,
- náhľad `scad/zostava_v_tekvici.scad` cez F5,
- funkčnosť všetkých relatívnych odkazov v `README.md`,
- že schémy zodpovedajú pinom vo firmvéri,
- bezpečný pohyb jednotlivých serv pomocou `S=2` až `S=5`.

