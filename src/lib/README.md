# Documentazione Libreria Recs

La libreria `ffmpeg_cpp` è basata sulle funzioni di coding end decoding di `libav`, 
scritte in C ma facilmente esportabili in C++ con in binding extern.

La struttura della cartella `lib` è la seguente:

```
.
├── ffmpeg_cpp
│   ├── include
│   │   ├── ffmpeg_cpp.h
│   │   ├── ffmpegc.h
│   │   └── ...
│   ├── Codec.cpp
│   ├── ffmpeg_cpp.cpp
│   ├── Format.cpp
│   ├── Frame.cpp
│   ├── Packet.cpp
│   ├── platform.cpp
│   └── Rescaler.cpp
├── include
│   └── ...
├── Recorder.h
└── Recorder.cpp
```

## Recorder

L'utente finale ha la possibilità d'includere `Recorder.h` 
dalla libreria per usare le funzionalità della classe `Recorder` che espone
le funzioni d'inizializzazione e di controllo della registrazione.

Un semplice esempio dell'uso della classe è il seguente:

```c++
auto rec = Recorder{};
rec.capture();
sleep(10);
rec.terminate();
```

La classe usa il paradigma RAII per liberare le risorse e il builder pattern
per impostare i parametri di registrazione. Di default viene catturato l'intero
schermo con codec video `h264` e codec audio `aac`, inoltre per impostare facilmente
i parametri dello schermo si usa la classe `Screen` anch'essa col builder pattern, ad esempio:

```c++
auto s = Screen{};
s.set_dimension("1920x1080");
s.set_offset("10,10");

auto r = Recorder{};
r.set_screen_param(s);
r.set_video_codec("mpeg4");
r.set_destination("file_prova.mp4");

r.capture();
sleep(10);
r.terminate();
```

Questo registra un'area di `1920x1080` pixel con offset di `(10,10)` pixel
dall'angolo in alto a sx dello schermo e salva il video di durata 10 secondi nel file `file_prova.mp4`
nella cartella corrente.

Da notare che la funzione `capture()` rilascia automaticamente dei thread per
la registrazione audio e vide; il numero dei thread è automaticamente calcolato
dal numero di core della cpu con `std::thread::hardware_concurrency()`, ciò non vieta
però all'utente di scegliere il profilo di esecuzione della cattura con `set_low_profile()` e
`set_high_profile()` che rilasciano rispettivamente due e quattro thread per la cattura.

Per interazione asincrona con i thread di cattura sono esposte delle funzioni di controllo:
`pause()` e `is_paused()` per mettere in pausa e controllare che la cattura sia in pausa,
`resume()` per riprendere dopo un comando di pausa e `is_capturing()` che controlla che sia
stato avviato il muxing/demux e quindi sia finita la fase di inizializzazione.

Le funzioni private della classe sono per lo più per l'inizializzazione e l'implementazione
dell'algoritmo di codifica e decodifica. In particolare nella funzione `init()` vengono 
inizializzate e allocate le strutture dati di ffmpeg per la cattura e nella funzione di `reset()`
vengono rilasciate le risorse e reinizializzati i dati per una nuova cattura.


###Thread asincroni per la cattura audio/video



###Controllo e gestione errori durante esecuzione dei thread asincroni

Nel momento in cui uno qualsiasi dei thread audio/video di cattura/demuxing/conversione lancia
internamente un'eccezione, questa viene automaticamente gestita dal thread in questione,
che si preoccuperà di avviare un thread secondario che esegue la funzione `handle_rec_error`
la quale riceve come parametri il nome del thread, un numero che lo identifica per la sua
funzione e una stringa che specifica il tipo di errore che si è verificato.

La funzione si occupa quindi di salvare i dati relativi all'errore in una variabile interna
alla classe e di interrompere l'esecuzione dei thread audio/video, oltre che di ripristinare
lo stato delle strutture dati interne. 
Dopodichè il thread secondario termina e sarà possibile accedere ai dati relativi all'errore
verificatosi tramite una chiamata al metodo pubblico `get_exec_error` che riceve tramite
riferimento una variabile bool nella quale viene memorizzato l'effettivo avvenire di un errore
durante la fase di registrazione entro il momento della chiamata del metodo e restituisce una
stringa con i dettagli sull'errore in caso affermativo.


[//]: # (todo funzioni di cattura)

## Template `wrapper`

## Codec

## Format

## Rescaler

## Template `ptr_wrapper`

## Packer e Frame
