# Documentazione Libreria Recs

La libreria `ffmpeg_cpp` è basata sulle funzioni di coding end decoding di `libav`, scritte in C ma facilmente
esportabili in C++ con in binding extern.

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
dalla libreria per usare le funzionalità della classe `Recorder` che espone le funzioni d'inizializzazione e di
controllo della registrazione.

Un semplice esempio dell'uso della classe è il seguente:

```c++
auto rec = Recorder{};
rec.capture();
sleep(10);
rec.terminate();
```

La classe usa il paradigma RAII per liberare le risorse e il builder pattern per impostare i parametri di registrazione.
Di default viene catturato l'intero schermo con codec video `h264` e codec audio `aac`, inoltre per impostare facilmente
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

Questo registra un'area di `1920x1080` pixel con offset di `(10,10)` pixel dall'angolo in alto a sx dello schermo e
salva il video di durata 10 secondi nel file `file_prova.mp4`
nella cartella corrente.

Da notare che la funzione `capture()` rilascia automaticamente dei thread per la registrazione audio e vide; il numero
dei thread è automaticamente calcolato dal numero di core della cpu con `std::thread::hardware_concurrency()`, ciò non
vieta però all'utente di scegliere il profilo di esecuzione della cattura con `set_low_profile()` e
`set_high_profile()` che rilasciano rispettivamente due e quattro thread per la cattura.

Per interazione asincrona con i thread di cattura sono esposte delle funzioni di controllo:
`pause()` e `is_paused()` per mettere in pausa e controllare che la cattura sia in pausa,
`resume()` per riprendere dopo un comando di pausa, `is_capturing()` che controlla che sia stato avviato il muxing/demux
e quindi sia finita la fase di inizializzazione e infine `terminate()` che permette di terminare la cattura.

Le funzioni private della classe sono per lo più per l'inizializzazione e l'implementazione dell'algoritmo di codifica e
decodifica. In particolare nella funzione `init()` vengono inizializzate e allocate le strutture dati di ffmpeg per la
cattura e nella funzione di `reset()`
vengono rilasciate le risorse e reinizializzati i dati per una nuova cattura.


### Thread asincroni per la cattura audio/video

In base alla quantità di thread fisicamente presenti, la classe Recorder si occupa di decidere tra due tipi di
esecuzione differenti: la prima consiste nell'adoperare due thread di cattura, uno `th_audio` per il flusso audio ed un
altro `th_video` per il flusso video, pensata per sistemi con fino a due thread fisici; la seconda invece separa i
compiti di cattura in demuxing e conversione/scrittura per ogni flusso audio/video, per un totale di quattro thread in
esecuzione, ovvero `th_audio_demux`, `th_audio_convert`, `th_video_demux` e `th_video_convert`.

Più in dettaglio:

- `th_audio`:
  esegue la funzione `CaptureAudioFrames`, che mantiene un ciclo while di lettura dei pacchetti audio in ingresso fino a
  che non viene convalidata una condizione di terminazione, che può portare allo stop della
  regitrazione (`stopped == true`) o alla terminazione immediata della funzione (`capturing == false`).

  In caso durante il ciclo `pausing == true`, imposta la condizione di pausa e segnala la funzione che ha richiesto la
  pausa. In tal modo, i frame vengono estratti dallo stream di ingresso ma non vengono processati ulteriormente.
  Similarmente, lo stesso accade per il caso in cui `resuming == true`, nella quale invece svuota i buffer d'ingresso e
  rimuove la condizione di pausa.

  Dunque, se la registrazione non è in pausa, invia i pacchetti alla funzione di decodifica
  `decode` e successivamente a quella di conversione formato sample e scrittura su file
  `convertAndWriteAudioFrames`.

  Al termine del ciclo viene chiamata una funzione `convertAndWriteLastAudioFrames` che si occupa di convertire e
  scrivere sul file di uscita tutti i pacchetti rimanenti nella coda interna.


- `th_video`:
  esegue la funzione `CaptureVideoFrames`, che mantiene un ciclo while di lettura dei pacchetti audio in ingresso fino a
  che non viene convalidata una condizione di terminazione, che può portare allo stop della
  registrazione (`stopped == true`) o alla terminazione immediata della funzione (`capturing == false`).

  In caso durante il ciclo `pausing == true`, imposta la condizione di pausa e segnala la funzione che ha richiesto la
  pausa. In tal modo, i frame vengono estratti dallo stream di ingresso ma non vengono processati ulteriormente.

  Similarmente, lo stesso accade per il caso in cui `resuming == true`, nella quale invece svuota i buffer d'ingresso e
  rimuove la condizione di pausa. Dunque, se la registrazione non è in pausa, invia i pacchetti alla funzione di
  decodifica
  `decode` e successivamente a quella di conversione formato sample e scrittura su file
  `convertAndWriteVideoFrame`.

  Al termine del ciclo viene chiamata una funzione `convertAndWriteDelayedVideoFrames` che si occupa di convertire e
  scrivere sul file di uscita tutti i pacchetti rimanenti nella coda interna.


- `th_audio_demux`:
  esegue la funzione `DemuxAudioInput`, che mantiene un ciclo while di lettura dei pacchetti audio in ingresso fino a
  che non viene convalidata una condizione di terminazione, che può portare allo stop della
  registrazione (`stopped == true`) o alla terminazione immediata della funzione (`capturing == false`), con
  impostazione del flag `finishedAudioDemux = true` e segnalazione al thread di conversione che potrebbe essere rimasto
  in attesa di nuovi pacchetti.

  In caso durante il ciclo `pausing == true`, imposta la condizione di pausa e segnala la funzione che ha richiesto la
  pausa. In tal modo, i frame vengono estratti dallo stream di ingresso ma non vengono processati ulteriormente.

  Similarmente, lo stesso accade per il caso in cui `resuming == true`, nella quale invece svuota i buffer d'ingresso e
  rimuove la condizione di pausa.

  Dunque, se la registrazione non è in pausa, invia i pacchetti alla funzione `avcodec_send_packet`
  che inserisce i pacchetti nella coda di decodifica e segnala al thread di conversione l'avvenuto inserimento in caso
  di invio avvenuto con successo.

  In caso contrario, se la coda di decodifica è piena si mette in attesa di un segnale da parte del thread di
  conversione, altrimenti viene generata un'eccezione.


- `th_video_demux`:
  esegue la funzione `DemuxVideoInput`, che mantiene un ciclo while di lettura dei pacchetti audio in ingresso fino a
  che non viene convalidata una condizione di terminazione, che può portare allo stop della
  registrazione (`stopped == true`) o alla terminazione immediata della funzione (`capturing == false`), con
  impostazione del flag `finishedVideoDemux = true` e segnalazione al thread di conversione che potrebbe essere rimasto
  in attesa di nuovi pacchetti.

  In caso durante il ciclo `pausing == true`, imposta la condizione di pausa e segnala la funzione che ha richiesto la
  pausa. In tal modo, i frame vengono estratti dallo stream di ingresso ma non vengono processati ulteriormente.

  Similarmente, lo stesso accade per il caso in cui `resuming == true`, nella quale invece svuota i buffer d'ingresso e
  rimuove la condizione di pausa.

  Dunque, se la registrazione non è in pausa, invia i pacchetti alla funzione `avcodec_send_packet`
  che inserisce i pacchetti nella coda di decodifica e segnala al thread di conversione l'avvenuto inserimento in caso
  di invio avvenuto con successo.

  In caso contrario, se la coda di decodifica è piena si mette in attesa di un segnale da parte del thread di
  conversione, altrimenti viene generata un'eccezione.


- `th_audio_convert`:
  esegue la funzione `ConvertAudioFrames`, che mantiene un ciclo while di lettura dei pacchetti audio decodificati
  tramite la funzione `avcodec_receive_frame` fino a che non viene convalidata una condizione di terminazione, che può
  portare allo stop della registrazione
  (`finishedAudioDemux == true`) o alla terminazione immediata della funzione (`capturing == false`).

  Se i pacchetti decodificati vengono ricevuti con successo, questi vengono processati dalla funzione
  `convertAndWriteAudioFrames` che si occuperà di effettuare resampling, encoding e scrittura su file.

  Altrimenti, se la coda di decodifica è vuota, si mette in attesa di un segnale da parte del demuxer. Nel caso siano
  presenti altri tipi di errore viene lanciata un'eccezione.

  Al termine del ciclo viene chiamata la funzione `convertAndWriteLastAudioFrames` che si occupa di svuotare le code
  interne scrivendo i frame rimasti su file.


- `th_video_convert`:
  esegue la funzione `ConvertVideoFrames`, che mantiene un ciclo while di lettura dei pacchetti audio decodificati
  tramite la funzione `avcodec_receive_frame` fino a che non viene convalidata una condizione di terminazione, che può
  portare allo stop della registrazione
  (`finishedVideoDemux == true`) o alla terminazione immediata della funzione (`capturing == false`).

  Se i pacchetti decodificati vengono ricevuti con successo, questi vengono processati dalla funzione
  `convertAndWriteVideoFrame` che si occuperà di effettuare resampling, encoding e scrittura su file.

  Altrimenti, se la coda di decodifica è vuota, si mette in attesa di un segnale da parte del demuxer. Nel caso siano
  presenti altri tipi di errore viene lanciata un'eccezione.

  Al termine del ciclo viene chiamata la funzione `convertAndWriteDelayedVideoFrames` che si occupa di svuotare le code
  interne scrivendo i frame rimasti su file.

### Funzioni di conversione/scrittura

All'interno del file `ffmpeg_cpp.cpp` sono presenti varie funzioni che vengono adoperate dai thread durante le varie
fasi di conversione e scrittura su file dei frame/pacchetti.

- `decode`: inserisce i pacchetti in una coda di decodifica da cui estrae, se possibile, i frame decodificati.
  Lancia `avException("Failed to send packet to decoder")` in caso di errore.


- `encode`: inserisce i frame in una coda di codifica da cui estrae, se possibile, i pacchetti codificati.
  Lancia `avException("Failed to send frame to encoder")` in caso di errore.


- `writeFrameToOutput`: scrive in modo thread safe nell'uscita desiderata. Lancia `avException("Provided null output
values, data could not be written")` se riceve dei parametri `null` e `avException("Error in writing media frame")`
nel caso in cui avvengano degli errori durante la scrittura.


- `convertAndWriteVideoFrame`: si occupa di chiamare le funzioni opportune per effettuare rescaling, codifica e scrittura
in uscita di un frame video. Eventuali eccezioni generate da queste funzioni si propagano al chiamante. Se richiesto,
viene effettuata resincronia del pts dei frame in base ai valori forniti.


- `convertAndWriteDelayedVideoFrames`: estrae i pacchetti rimasti nella coda di codifica e chiama la funzione opportuna
per scrivere i pacchetti in uscita. Eventuali eccezioni generate da questa funzione di scrittura si propagano al
chiamante.


- `convertAndWriteAudioFrames`: si occupa di chiamare le funzioni opportune per effettuare resampling, codifica e 
scrittura in uscita dei dati presenti in un frame audio d'ingresso. Eventuali eccezioni generate da queste funzioni si
propagano al chiamante. Se richiesto, viene effettuata resincronia del pts dei frame in base ai valori forniti.


- `convertAndWriteDelayedAudioFrames`: estrae i pacchetti rimasti nella coda di codifica e chiama la funzione opportuna
per scrivere i pacchetti in uscita. Eventuali eccezioni generate da questa funzione di scrittura si propagano al
chiamante.


- `convertAndWriteLastAudioFrames`: estrae i pacchetti rimasti nella coda di resampling, li codifica e chiama la
funzione opportuna per scrivere i pacchetti in uscita. Eventuali eccezioni generate da queste funzioni si propagano al
chiamante. Se richiesto, viene effettuata resincronia del pts dei frame in base ai valori forniti.


### Controllo e gestione errori durante l'esecuzione dei thread asincroni

Nel momento in cui uno qualsiasi dei thread audio/video di cattura/demuxing/conversione lancia internamente
un'eccezione, questa viene automaticamente gestita dal thread in questione, che si preoccuperà di avviare un thread
secondario che esegue la funzione `handle_rec_error`
la quale riceve come parametri il nome del thread, un numero che lo identifica per la sua funzione e una stringa che
specifica il tipo di errore che si è verificato.

La funzione si occupa quindi di salvare i dati relativi all'errore in una variabile interna alla classe e di
interrompere l'esecuzione dei thread audio/video, oltre che di ripristinare lo stato delle strutture dati interne.

Dopodichè il thread secondario termina e sarà possibile accedere ai dati relativi all'errore verificatosi tramite una
chiamata al metodo pubblico `get_exec_error` che riceve tramite riferimento una variabile bool nella quale viene
memorizzato l'effettivo avvenire di un errore durante la fase di registrazione entro il momento della chiamata del
metodo e restituisce una stringa con i dettagli sull'errore in caso affermativo.

Durante l'esecuzione dei thread audio/video le eventuali eccezioni lanciate non si propagano direttamente all'esterno
della struttura Recorder ma eventuali condizioni d'errore devono essere monitorate periodicamente durante l'esecuzione
tramite `get_exec_error`;

### Condizioni di errore

- **Fase di inizializzazione della registrazione**: durante l'esecuzione della funzione `init()` possono essere lanciate
eccezioni dovute (solitamente) a errori legati a codec mancanti o errata inizializzazione di ingressi o uscite (file
di uscita), al seguito dei quali sarà necessario distruggere l'oggetto e inizializzarne uno nuovo.
 

- **Fase di esecuzione della registrazione**: durante l'esecuzione dei thread audio/video si possono verificare condizioni
di errore dovute, eccetto casi imprevisti, a problemi nelle operazioni di codifica/decodifica, rescaling/resampling e
scrittura su file. Per verificare se sia avvenuto un errore in questa fase è necessario adoperare il metodo
`get_exec_error` a cui si passa un valore `bool` per riferimento nel quale viene scritta l'effettiva avvenuta di
eventuali errori e che ritorna in caso positivo una `std::string` con il relativo messaggio d'errore.
È sufficiente, in caso di errore (salvo casi catastrofici), effettuare una nuova `capture()` in quanto internamente le
strutture dati dell'oggetto vengono automaticamente deinizializzate/ripristinate, ma è ugualmente possibile distruggere
l'oggetto e crearne uno nuovo.


- **Fase di pausa/ripresa della registrazione**: non vengono eseguite funzioni che normalmente generano eccezioni. 
Nel caso si verifichino errori (gravi) imprevisti, sarà necessaria la sua distruzione dell'oggetto Recorder.


- **Fase di terminazione della registrazione**: durante la fase di terminazione possono verificarsi  eccezioni dovute ad
errori nell'accesso al file in uscita. In caso di errore sarà necessaria la distruzione dell'oggetto Recorder.


- **Fase di salvataggio/cancellazione del file finale**: durante questa fase non viene adoperata la classe Recorder ma
la funzione `move_file` che si occupa di spostare il file temporaneo di uscita alla destinazione richiesta in caso di
salvataggio. In caso di annullamento, il file viene eliminato dalla funzione `delete_file`.
Entrambe le funzioni posso lanciare eccezioni dovute ad errori nell'accesso a file e/o filesystem o al passaggio di
parametri non corretti.

## Template `wrapper`

`wrapper` è un interfaccia che espone metodi di get e set dei puntatori nativi usati da `libav` di `ffmpeg`, implementa
delle specializzazioni, per alcuni tipi, per il rilascio della memoria seguendo il paradigma RAII. Non è pesata per
essere usata direttamente dall'utente, ma come wrapper, appunto per attributi nelle classi `Codec` e `Format`.

## Codec

`Codec` è la classe che alloca le risorse per `AVCodec` e `AVCodecContext` per l'input e l'output, oltre che
agli `AVStream` di audio e video. Nelle funzioni d'inizializzazione vengono usati parametri di default (non dipendenti
dalla piattaforma) e i parametri platform specific vengono ricavati con delle funzioni di `platform.cpp` con
compilazione condizionale in base all'OS.

## Format

`Format` è la classe che alloca le risorse per `AVInputFormat`, `AVOutputFormat` e `AVFormatContext`, rilevando
automaticamente i device e gli input di registrazione supportati. Il parametro `outputContext` è usato per il context
del file di destinazione, e viene allocato e deallocato con funzioni diverse rispetto ad `inputContext`.

## Rescaler

`Rescaler` è una classe wrapper di `SwsContext` e `SwrContext` che nelle funzioni di cattura servono per la conversione
del formato pixel video e per il resampling audio, rispettivamente. La classe alloca e dealloca la memoria con paradigma
RAII ed espone i puntatori con dei metodi get/set.

## Packet e Frame

Classi wrapper di `AVPacket` e `AVFrame`, per implementare il RAII su questi tipi e quindi poter essere usati dentro un
ciclo, senza dover deallocare manualmente. Implementano il template `ptr_wrapper` per le funzioni `into()`, che ritorna
il puntatore interno per le funzioni di `libav`, e `unref()` per dereferenziare manualmente il puntatore, ma non
rilasciare la memoria, necessario per alcune funzioni di `libav` che si occupano automaticamente di gestire poi la
memoria.
