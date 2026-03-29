Generowanie zbioru Mandelbrota - program typu Manager - Workers (Quinn, zad. 9.9)

architektura: liczba węzłów pracujących równolegle dopasowana do natury rozwiązywanego problemu i rozmiaru przetwarzanych danych,
implementacja: praca w środowisku pracowni komputerowej MPICH, OpenMPI -- plus opcjonalnie dodatkowo wykorzystanie OpenMP, CUDA,
    kod źródłowy programu,
    plik z przykładowymi danymi wejściowymi,
    plik w latex z opisem budowy, działania i obsługi programu (zawierającym m. in. schemat blokowy)
    – w formacie PDF,
    plik Makefile z regułami umożliwiającymi:
    - kompilację programu źródłowego do postaci wykonywalnej na komputerach pracowni,
    - uruchomienie programu z danymi przykładowymi,
    - przywrócenie zawartości podkatalogu do stanu wyjściowego.

Realizacja projektu 45 pkt
    realizacja logiki aplikacji - 25 pkt,
    realizacja interfejsu użytkownika - 5 pkt,
    dokumentacja projektu (w tym kodu źródłowego) - 10 pkt,
    prezentacja działania projektu - 5 pkt.


config.txt:
- szerokość (px), 
- wysokość (px), 
- xmin​, 
- xmax​, 
- ymin​, 
- ymax​, 
- maksymalna liczba iteracji.