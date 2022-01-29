# Recs

Sowftware la cattura e registrazione dello schermo. 
Sono supportati i seguenti sistemi:

- Linux
- Windows
- MacOS

Il software si compone di una libreria per la registrazione dello schermo
e di una interfaccia grafica per una facile gestione.

La struttura del progetto è la seguente:

```text
.
├── assets
└── src
    ├── Interface.cpp
    ├── main.cpp
    └── lib
        ├── Recorder.cpp
        ├── ...
        └── ffmpeg_cpp
```

- In `asset` sono contenuti gli asset grafici per la gui
- In `src` ci sono tutti i sorgenti e i file di setup per `cmake`
- In `lib` la libreria di recording con binding alle funzioni di [ffmpeg](https://ffmpeg.org/) e
una sua implementazione con classi e idioma RAII ([qui](src/lib/README.md) la documentazione dettagliata)
- `Interface.cpp` implementa una semplice interfaccia grafica con il framework [GTK4](https://docs.gtk.org/gtk4/getting_started.html)

## Build

Per il del progetto sono necessarie le seguenti dipendenze:

- `cmake 3.20+`
- `clang 13+`
- `libav 4.4.1+`  <!-- TODO Controllare questo -->

Per quanto riguarda `libav`, su Linux le dipendenze sono automaticamente scaricate col pacchetto `ffmpeg`,
su Windows ... <!-- TODO aggiungere installazione -->
su MacOS ... <!-- TODO se è uguale a linux -->

Dalla cartella root del progetto eseguire il build con:

```shell
cmake -B build
cd build
make
```

Questo genererà l'eseguibile nella cartella `build` appena creata, eseguire l'applicazione con:

```shell
./recs
```

## Authors

- Belluardo Gabriele s290270 
- Bottisio Alessandro s277945

## License

[GPL-3.0+](LICENSE)

