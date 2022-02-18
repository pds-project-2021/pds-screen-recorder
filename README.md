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
- `clang 13+` o `MSVC c++ compiler`
- `ffmpeg 4.4.1+`
- `gtk4`
- `libx264`

Installare i pacchetti `ffmpeg`, `gtk4`, e `libx264` è il modo più veloce per soddisfare automaticamente le dipendenze, tramite un package manager su sistemi Linux oppure adoperando tool come vcpkg per sistemi Windows o brew per MacOS. 

Dalla cartella root del progetto eseguire il build con:

```shell
cmake -B build
cd build
make
```

Questo genererà l'eseguibile nella cartella `build` appena creata.

###Procedure aggiuntive per sistemi Windows
Nel caso in cui si effettui la build su un sistema Windows, è necessario disporre delle dipendenze richiste nel file cmake, che è stato configurato per integrarsi
con eventuali installazioni presenti di vcpkg (il percorso di installazione richiesto è "C:/src/vcpkg" ma può essere modificato nel file per soddisfare percorsi differenti).

Sarà inoltre necessario aggiungere -DCMAKE_TOOLCHAIN_FILE={vcpkg-root-folder}/scripts/buildsystems/vcpkg.cmake alle opzioni di cmake sostituendo la parte tra parentesi con il percorso di installazione di vcpkg.

A questo punto tutte le dipendenze necessarie possono essere installate tramite vcpkg e verranno riconosciute automaticamente.

Sarà necessario aggiungere alle variabili di ambiente XDG_DATA_DIRS={vcpkg-root-folder}\installed\x64-windows\share per permettere il corretto funzionamento di gtk4 nel caso di installazione tramite vcpkg.

##Esecuzione
Eseguire con `./recs`, si possono aggiunger dei flag da linea di comando:

```shell
USAGE: recs <args>
  -v              Set logging to INFO level
  -w              Set loggign to DEBUG level
  -V, --version   Print current software version
  -h, --help      Show this helper
```

## Authors

- Belluardo Gabriele s290270 
- Bottisio Alessandro s277945

## License

[GPL-3.0+](LICENSE)

