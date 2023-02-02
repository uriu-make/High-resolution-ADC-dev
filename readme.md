# High-resolution-ADC-dev
Rsapberry PiとADS1256を使用した測定システムのRaspberry Pi側ソフトウェア
## 開発環境
Raspberry PiはRaspberry Pi 3 model Bを使用し、OSはRaspberry Pi OS Bullseye 64bitを使用した。

Raspberry Pi側のプログラムの開発環境はDockerコンテナ上に構築した。
コンテナはRaspberry Pi OSの最新版のベースであるDebian11を使用し、Linux-ARM64のクロスコンパイラを使うことでRaspberry Pi上で動作するソフトウェアをビルドした。ビルドしたファイルなどの共有はrsyncを使用し、ssh経由で共有した。\
また、Raspberry Piにgdbserverを、Dockerコンテナにgdb-multiarchをインストールすることで、Visual Studio Codeを使用したデバッグ環境の構築を行った。
### ハードウェア
Waveshare製のRaspberry Pi High-Precision AD/DA Expansion Boardを使用した。\
この基板ではADCにADS1256を、DACにはDAC8552を使用している。また、SPIのCSがRaspberry Piのデフォルトのものと異なるため、`/boot/config.txt`に
~~~
dtoverlay=spi0-2cs,cs0_pin=22,cs1_pin=23
~~~
を追記する。\
また、測定用のプログラムがその他のタスクによる影響で遅延することを防ぐために、`/boot/cmdline.txt`の末尾に改行せずに
~~~
isolcpus=2-3
~~~
を追記することで、カーネルのスケジューラから指定したCPUを分離させ、特定のタスク専用のCPUとなるようにした。

### プログラム
ADS1256はSPIとGPIOとを使用する必要がある。これらの操作はWiringPiなどのライブラリを使うのではなく、Linuxのシステムコールでデバイスドライバを直接操作する方法で行った。これは、ADS1256のSPI通信は通信中に待機する時間を設けるなどの設定が必要なため、細かな設定を触ることができる手法が必要だったからである。\
ADS1256で、測定結果の取得が可能かはDRDYピンの状態を読み取ることで確認できる。これはLinuxのGPIOイベントの検出を使用して実装した。GPIOイベントとはGPIOピンの状態がHigh->LowまたはLow->Highに変化したことを指す。

プログラムはC++を使用した。プログラムはまずソケット通信するPCとの接続を待つ。接続が確立されると、ADS1256と通信するスレッド、ソケット通信でPCにデータを送信するスレッド、ソケット通信でPCからの命令を受信するスレッドを生成する。各スレッドは割り当てられたCPUでのみ動作する。このとき、共有する資源は排他制御されるが、通常のC++のmutexではなくスピンロックを使用する。mutexはロックされていた場合、プログラムをそこで停止させるが、スピンロックはそこでループ処理を行う。これによりロックが解除されてからの復帰が早いがその間CPUの使用率が100%になるデメリットがある。しかし、待機時間の短い場合は有効な選択となる。\
プログラムはマルチスレッドで動作するようにした。スレッドはC++20で追加されたjthreadを採用した。jthreadはスレッド外から明示的にそのスレッドを終了させることができる機能がある。今回は例外が発生したときに各スレッドを停止させるために使用した。\
各スレッドの機能は、ソケット通信の受信を担当するスレッド、ADCと通信するためのスレッド、データの処理とソケット通信を行うスレッドの3つの子スレッドと、ADCやソケット通信の初期化を行う親スレッドで構成されている。子スレッドのうち、1つ目以外の2つのスレッドはスケジューラから分離したCPU上で動作するようにした。

## プロトコル
PCとRaspberry Piはソケット通信を使用する。通信では構造体を使用している。\
設定を送信する構造体は以下の通り
~~~cpp
struct COMMAND {
  uint8_t rate;     //測定間隔
  uint8_t gain;     //PGA
  uint8_t positive; //正入力
  uint8_t negative; //負入力
  uint8_t buf;      //アナログバッファの有効化/無効化
  uint8_t sync;     //同期を取る機能の有効化/無効化
  uint8_t mode;     //通常の測定かFFTかの選択
  uint8_t run;      //測定開始/停止
  uint8_t kill = 0; //プログラムを終了
};
~~~
rate,gain,positive,negativeは`ADS1256.h`で定義されている数値を使用し、modeは0で通常の測定、1でFFTになる。それ以外は0で無効、それ以外で有効となる。

通常の測定で送られるデータは以下のとおりである。
~~~cpp
struct read_data {
  int32_t len;            //有効な要素数
  double volt[SAMPLELEN]; //電圧
  uint64_t t[SAMPLELEN];  //時間
};
~~~
データは固定長の配列に格納され、SAMPLELENは`ADS1256.h`で定義されている。\
Raspberry Piでは等間隔に処理できないため、送信可能なデータ数は不定である。また、測定間隔も最も遅いもので毎秒2.5サンプルのため、データが一定数貯まると送信するなどの方法は使いづらい。そこで、固定長の配列から有効な要素数だけコピーする方法を採用した。

FFTで送られるデータは以下のとおりである。
~~~cpp
struct fft_data {
  double freq_bin;            //周波数ビン
  std::complex<double> F[N];  //FFTの結果
};
~~~
FFTではデータ数が固定であることが自明であるため、そのまま送信される。また、後に逆フーリエ変換をすることを考慮し、複素数のまま送信する。\
freq_binは周波数ビンのことで、FFTが何Hz刻みで測定されているかを示す。