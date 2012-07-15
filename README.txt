-*- outline -*-

http://d.hatena.ne.jp/propella/20120715/p1

*[工作] Aruino Ethernet でテクノ手芸。白イルカにネットワーク監視をさせる。

<a href="http://www.flickr.com/photos/propella/7549049268/" title="P1030398 by propella, on Flickr"><img src="http://farm9.staticflickr.com/8023/7549049268_6b2b0e6ff8.jpg" width="500" height="375" alt="P1030398"></a>

弊社 UIEvolution ではソースコードの管理に Perforce というツールを使っております。いつでもコードの過去に遡れる優れものですが、一つ難点がありまして、毎晩メンテのために止まるのです。普段はそれくらい問題ないのですが、たまーにリリース前でイライラしている時に止まると腹がたって精神衛生上非常によろしくない。そこで、Arduino Ethernet を使いかわいくサービス停止中のお知らせを伝える仕組みというのを作ってみました。以下ソースは https://github.com/propella/netPeek にあります。

** 作るもの

とあるウェブサイトにアクセスして、その内容にとある文字列が含まれていなければ LED を光らせます。この仕組を応用すると色々なネットワーク監視装置を作る事が出来ます。

** 必要な知識

- Arduino に LED を繋げて点滅が出来る。
- CGI が書ける。
- 裁縫が出来る。

** システム構成

<a href="http://www.flickr.com/photos/propella/7574638688/" title="diagram by propella, on Flickr"><img src="http://farm9.staticflickr.com/8284/7574638688_8a55c5881c.jpg" width="500" height="375" alt="diagram"></a>

ざくっと構成についてご説明します。目的は監視対象となるサーバーの動きを定期的に監視して何か起こったらぬいぐるみの目に仕込んだ LED を光らせるというだけです。でもちょっとややこしいのは、 Arduino Ethernet 自体は Web にアクセスする事くらいしか出来なくて Perforce の監視には荷が重いのです。

そこで別に監視用 Web サーバーを作り Perforce の監視をしてもらいます。そいつは順調に動いている時は "PERFORCE-LOOKS-GOOD-AND-SMOOTH" という文字列を出力し、問題がある時は "SOMETHING-IS-TECHNICALLY-WRONG" と出します。Arduino は監視サーバーにアクセスし、応答に PERFORCE-LOOKS-GOOD-AND-SMOOTH が含まれて<b>いなければ</b>問題が発生したとみなし LED を光らせます。面倒くさいので HTTP ヘッダの解析はせず、応答文字列全てを strstr で比較します。応答はなんでも良いのですが、Web サーバーが出すヘッダ文字列に間違って反応すると困るので、"OK" とかじゃなくて"PERFORCE-LOOKS-GOOD-AND-SMOOTH" のような長めの文字にしておきます。では左から順に見てゆきましょう。

** 監視サーバー

もしも皆さんが Perforce のような特殊な物じゃなくて、単にウェブを監視したいだけならここは不要です。

監視サーバーは単なる CGI スクリプトです。"p4 revert -n ..." というコマンドを実行し、失敗したり 5 秒間応答が無いと "SOMETHING-IS-TECHNICALLY-WRONG" を出します。プログラム的には単純ですが、難しいのは監視に使えるコマンドを試行錯誤で編み出す事です。

>|python|
#!/usr/bin/env python
#
# P4 monitor Based on http://code.pui.ch/2007/02/19/set-timeout-for-a-shell-command-in-python/

def timeout_command(command, timeout, env):
    """call shell-command and either return its output or kill it
    if it doesn't normally exit within timeout seconds and return None"""
    import subprocess, datetime, os, time, signal
    start = datetime.datetime.now()
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
    while process.poll() is None:
        time.sleep(0.1)
        now = datetime.datetime.now()
        if (now - start).seconds> timeout:
            os.kill(process.pid, signal.SIGKILL)
            os.waitpid(-1, os.WNOHANG)
            return None
    return process.poll()

status = timeout_command(["/usr/local/bin/p4", "revert", "-n", "..."], 5, {"P4CONFIG":"p4config"})

if status == 0:
    response = "PERFORCE-LOOKS-GOOD-AND-SMOOTH"
else:
    response = "SOMETHING-IS-TECHNICALLY-WRONG"

print "Content-Type: Text/plain"
print
print response
||<

** Arduino Ethernet スケッチ

次は Arduino スケッチです。以下の例では、http://192.168.0.1/~tyamamiya/p4peek.cgi にアクセスして "PERFORCE-LOOKS-GOOD-AND-SMOOTH" という文字が含まれなければ LED を点滅します。

まず最初に文字列比較用のバッファとして 1024 バイト確保し、mac アドレスや監視サーバー IP 等を設定します。

>|c|
#include <SPI.h>
#include <Ethernet.h>
#include <string.h>

#define GOOD_MARKER "PERFORCE-LOOKS-GOOD-AND-SMOOTH"

#define MAX_BUFFER 1024

byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x29, 0x8E }; // My MAC address

IPAddress server(192,168,0,1); // Target IP

int index = 0;
char buffer[MAX_BUFFER];

// Initialize the Ethernet client library
EthernetClient client;
||<

setup では LED 出力用に 8 番ピン。動作確認に内蔵 9 番 LED を設定します。念のためデバッグ用のシリアルを設定し、Ethernet.begin() で DHCP が働き、自分の IP を勝手に設定してネット通信開始です。

>|c|
void setup() {

  pinMode(8, OUTPUT); // External LEDs
  pinMode(9, OUTPUT); // a built-in LED for Arduino Ethernet
  
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    while (true) {}
  }
}
||<

次に、実際にウェブにアクセス開始する部分です。以下のようにすると http://192.168.0.1/~tyamamiya/p4peek.cgi にアクセスします。

>|c|
void connect() {
  Serial.println("connecting...");
  
  if (client.connect(server, 80)) {
    Serial.println("connected");
    client.println("GET /~tyamamiya/p4peek.cgi HTTP/1.0");
    client.println();
  } 
  else {
    Serial.println("connection failed");
  }
}
||<

LED を点滅する部分です。

- もしもサーバーが止まっていたら (isOn == true) ピン 8 の目の LED が点滅し、
- 普通の時は (isOn == false) 基板上にあるピン 9 の小さい LED が点滅します。

普段内蔵 LED を点滅しておかないと動いてるのかどうかわからなくなるのでこうしました。

>|c|
void blink(boolean isON, int n) {
  Serial.print("Status: ");
  Serial.println(isON);
  // Repeat blink 5 times if the connection found
  for (int i = 0; i < n; i++) {
    digitalWrite(8, LOW);
    digitalWrite(9, LOW);
    delay(1000);
    digitalWrite(8, isON ? HIGH : LOW);
    digitalWrite(9, isON ? LOW : HIGH);
    delay(1000);
  }
}
||<

メインループです。ちょっとややこしいですが、もしもネットに接続していなければすでにバッファにウェブからの出力が溜まったとみなして strstr で内容を比較します。その結果に応じて blink() を呼び出し 2 秒 x 5 回 LED を点滅します。その後新たに接続を開始します。

もしも接続中ならサーバーから一文字読んでバッファに格納して、すぐに loop から抜けます。というのも、Arduino にはマルチスレッドが無くイベントドリブンで書くのですが、ネットへのアクセスは loop の外で暗黙のうちに行われるので用事を済ましたら loop を抜けないと行けません。これでスケッチは完成です。

>|c|
void loop()
{
  if (!client.connected() || index >= MAX_BUFFER - 1) {
    Serial.println("Connection finished.");
    buffer[index] = '\0';
    boolean found = strstr(buffer, GOOD_MARKER) != NULL;
    Serial.println(buffer);
    index = 0;
    Serial.println();
    Serial.println("reset connection.");
    client.stop();
    blink(!found, 5);

    connect(); // Connect for next timesstep.
  }

  // Read and record from the server
  if (client.available()) {
    char c = client.read();
    buffer[index] = c;
    index++;
  }
}
||<

** ぬいぐるみ

<a href="http://www.flickr.com/photos/propella/7549035456/" title="P1030345 by propella, on Flickr"><img src="http://farm9.staticflickr.com/8165/7549035456_3d5c524d6c_n.jpg" width="320" height="240" alt="P1030345"></a>

さてお待ちかね表示装置の制作です。今回手を抜いて『アニマル倶楽部 イルカさん(白)』というキットをそのまま使いました。

<a href="http://www.flickr.com/photos/propella/7549036104/" title="P1030350 by propella, on Flickr"><img src="http://farm9.staticflickr.com/8284/7549036104_0d24206160_n.jpg" width="320" height="240" alt="P1030350"></a> <a href="http://www.flickr.com/photos/propella/7549036996/" title="P1030352 by propella, on Flickr"><img src="http://farm9.staticflickr.com/8142/7549036996_b44b7b8366_n.jpg" width="320" height="240" alt="P1030352"></a> <a href="http://www.flickr.com/photos/propella/7549038422/" title="P1030354 by propella, on Flickr"><img src="http://farm9.staticflickr.com/8002/7549038422_4e6712155d_n.jpg" width="320" height="240" alt="P1030354"></a>

まず目に穴をあけて LED を仕込みたいのですが、モケモケにならないよう予め布テープで補強しておきます。

<a href="http://www.flickr.com/photos/propella/7549040116/" title="P1030359 by propella, on Flickr"><img src="http://farm8.staticflickr.com/7252/7549040116_2030120a25_n.jpg" width="320" height="240" alt="P1030359"></a> <a href="http://www.flickr.com/photos/propella/7549040930/" title="P1030362 by propella, on Flickr"><img src="http://farm9.staticflickr.com/8153/7549040930_a286d444a3_n.jpg" width="320" height="240" alt="P1030362"></a>

あとでコードが通ることになる穴も、ハトメでしっかりしておきます。

<a href="http://www.flickr.com/photos/propella/7549041882/" title="P1030363 by propella, on Flickr"><img src="http://farm9.staticflickr.com/8421/7549041882_9eaa579de6_n.jpg" width="320" height="240" alt="P1030363"></a> <a href="http://www.flickr.com/photos/propella/7549042568/" title="P1030368 by propella, on Flickr"><img src="http://farm9.staticflickr.com/8010/7549042568_f667b56379_n.jpg" width="320" height="240" alt="P1030368"></a>

ぬいぐるみというのはこのように裏返して縫って後でひっくり返します。

<a href="http://www.flickr.com/photos/propella/7549043852/" title="P1030372 by propella, on Flickr"><img src="http://farm9.staticflickr.com/8159/7549043852_ebd0e85efe_n.jpg" width="320" height="240" alt="P1030372"></a>

LED はハンダ付けして収縮チューブで絶縁します。収縮チューブというのはドライヤーの熱を当てると縮む便利なチューブです。

<a href="http://www.flickr.com/photos/propella/7549045400/" title="P1030379 by propella, on Flickr"><img src="http://farm9.staticflickr.com/8143/7549045400_b181ddac03_n.jpg" width="320" height="240" alt="P1030379"></a> <a href="http://www.flickr.com/photos/propella/7549046236/" title="P1030383 by propella, on Flickr"><img src="http://farm9.staticflickr.com/8283/7549046236_09bc11430a_n.jpg" width="320" height="240" alt="P1030383"></a> <a href="http://www.flickr.com/photos/propella/7549047048/" title="P1030388 by propella, on Flickr"><img src="http://farm8.staticflickr.com/7270/7549047048_46ac4dc286_n.jpg" width="320" height="240" alt="P1030388"></a>

LED を目につけて、コードを腹部から出します。ここで今回失敗したのですが、コードを出す時に中で結び目を作っておくと抜けなくて良いと思います。それから綿やペレット(重り)を詰め、穴をまつり縫いします。綿は詰め過ぎないほうが柔らかくて癒し感がアップします。

<a href="http://www.flickr.com/photos/propella/7566248824/" title="NetPeek by propella, on Flickr"><img src="http://farm9.staticflickr.com/8291/7566248824_9d3d35b22b_n.jpg" width="320" height="308" alt="NetPeek"></a> <a href="http://www.flickr.com/photos/propella/7549048686/" title="P1030395 by propella, on Flickr"><img src="http://farm8.staticflickr.com/7247/7549048686_ac76a4a2ed_n.jpg" width="320" height="240" alt="P1030395"></a>

LED 二つくらいマイコンに直接繋げても動くかも知れませんが、勉強も兼ねてちゃんとトランジスタで増幅しました。これらをハンダ付けして適当な箱に入れて完成です。作業の手間としてはハードより監視 CGI の調整に結構時間がかかりました。おしまい。

<a href="http://www.flickr.com/photos/propella/7549050294/" title="P1030404 by propella, on Flickr"><img src="http://farm9.staticflickr.com/8158/7549050294_aa9861ae04.jpg" width="500" height="375" alt="P1030404"></a>

ステマですが、<a href="http://www.uievolution.co.jp/recruit">弊社 UIEvolution では人材を募集しています。</a>白イルカを見たい人は是非ご応募下さい。電子工作の知識は不要です。

** 参考

- Arduino Ethernet: http://d.hatena.ne.jp/propella/20120519/p1
- Arduino に LED を繋げる http://d.hatena.ne.jp/propella/20120519/p2
- 参考にしたスケッチ: http://arduino.cc/en/Tutorial/WebClient
- 参考にした Python スクリプト: http://code.pui.ch/2007/02/19/set-timeout-for-a-shell-command-in-python/
- 回路図エディタ: http://fritzing.org/
