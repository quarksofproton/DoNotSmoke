---
title: "DO NOT SMOKE — Master Game Design Document"
domain: 01-Projects
category: Do-Not-Smoke
status: canonical
tags: [gdd, game-design, jam, monolithic]
---

# DO NOT SMOKE — Master Game Design Document (GDD)
*GameUp Türkiye 48-Hour Game Jam | COP31 Climate Resilience & Action*

---

## 1. Vizyon, Kimlik ve Tematik Altyapı

### 1.1 Proje Kimliği ve Görsel Sembol
* **Oyun Adı:** DO NOT SMOKE
* **Tematik Odak & Kategoriler:** Karbon Ayakizi ve Bilinçli Tüketim | Yenilenebilir Enerji ve Sürdürülebilirlik
* **Görsel Sembol / Logo:** Klasik kırmızı "Sigara İçilmez" (No Smoking) çemberi; içerisinde sigara yerine tütmekte olan endüstriyel fabrika bacası silüeti yer alır. İklim krizine esprili, vurucu ve anında anlaşılan bir hiciv sunar.
* **Tür:** 3D Arcade Glider & Precision Flyby Targeting (Arcade Uçuş ve Nokta Atışı Bombardıman)
* **Kamera Açısı:** 3rd-Person Dinamik Ayrık Takip Kamerası (Decoupled Chase Cam)
* **Oturum Süresi:** 3 – 5 Dakika (3 Seviyeli kesintisiz arcade koşusu)
* **Hedef Platform:** PC (Windows 64-bit Standalone)
* **Geliştirme Motoru:** Unreal Engine 5 (AI-First C++ Core + Thin Blueprint Layer)
* **Çekirdek Ekip ve Roller:**
  * **Quark:** Lead Game Designer & Game Developer (Pawn Kinematics, Projectile Math, Splines, Level Architecture, Game Flow)
  * **Berf:** UI/UX Designer & Sound Designer (Flight HUD, COP31 Summary Screens, Audio Mix, SFX Suite)
  * **Barlas:** 3D Artist (Hero Glider, 3 Modular Chimneys, Private Jet Mesh, Environment Assets, Logo)

### 1.2 Tematik Çekirdek ve Kategori Entegrasyonu (COP31)
> *"Gökyüzündeki dumanı süpürmek yalnızca semptomu öteler; asıl iklim adaleti, musluğu kaynağında kapatmak ve bacaları 'Do Not Smoke' mührüyle durdurmaktır."*

Geleneksel iklim oyunları oyuncuyu çaresizce çöp toplayan pasif bir temizlik işçisi konumuna hapseder. *DO NOT SMOKE*, oyuncuyu kirliliğin ana kaynaklarına ve karbon saçan lüks tüketime (özel jetler) doğrudan müdahale eden cesur bir hava denetçisi yapar. Oyunun temel mekanik omurgası iki ana etki kategorisi üzerine kuruludur:

1. **Karbon Ayakizi ve Bilinçli Tüketim:**
   * **Kaynakta Kesme ve Kümülatif Karbon Metriği:** Seviye puanlaması ve 3 yıldız karnesi salınan kümülatif karbon miktarına ($\text{kg CO}_2/\text{sn}$) bağlıdır. Semptomları filtrelemek yerine emisyonun doğrudan bacasında mühürlenmesi şarttır.
   * **Aşırı Tüketime Müdahale:** Gökyüzündeki elitist aşırı tüketimin ve orantısız karbon salınımının küresel simgesi olan lüks özel jetler (private jets), oyundaki en dinamik hava tehdidi olarak konumlandırılmıştır.
2. **Yenilenebilir Enerji ve Sürdürülebilirlik:**
   * **Solar Glider (Güneş Planörü):** Fosil yakıtlı veya motorlu bir araç yerine tamamen temiz güneş enerjisiyle çalışan aerodinamik bir planör kullanılır.
   * **Güneş Koridorları (Sun Pockets):** İrtifa kazanmanın tek yolu gökyüzündeki açık güneş koridorlarından fotovoltaik şarj almaktır. Bacalar açık kaldıkça duman tavanı güneşi kapatır; bacalar mühürlendikçe temiz enerji ve sürdürülebilir gökyüzü yeniden kazanılır.

---

## 2. 3C Şartnamesi ve Kontrol Modeli (Character, Camera, Controls)

### 2.1 Character / Vehicle: Solar Glider (Güneş Planörü)
Planör geleneksel motorlu bir uçak veya ağır Chaos fizik simülasyonu değildir; `Tick` bazlı kinematik aerodinamik süzülüş, momentum, hız sönümleme ve yerçekimi dengesiyle hareket eder.

* **Süzülüş Fiziği (Kinematik Model & Saf Enerji Kaybı):** 
  * **Net İrtifa Kaybı Kuralı:** Planör düz uçuşta veya dalış-tırmanış manevralarında her zaman net bir irtifa kaybı yaşar (Glide Ratio: 20:1). Boost yokken planör her açıda sürekli batar (hız / 20; temel hız 2400 u/s'de ~120 u/s): düz uçuşta irtifa korunamaz, girdisiz bir planör spawn irtifasından (2200u) kısa sürede yere iner. İrtifa kazanmanın tek yolu güneşle şarj olup Solar Boost kullanmaktır. Dalıştan kazanılan hızla burnu yukarı çekmek kinetik enerjiyi irtifaya dönüştürür ancak asla dalış öncesi irtifanın üstüne çıkaramaz (sonsuz süzülüş veya kinetik perpetual-motion engellenmiştir).
  * **İrtifa Kazanımının Tek Yolu:** Planörün net irtifa kazanabilmesinin tek ve mutlak yolu **Solar Boost** harcamaktır.
  * **Dalış ve Tutunma:** Burnunu aşağı eğdikçe (Pitch Down) yerçekimi kinetik hıza dönüşür; hızlandıkça kontrol hassasiyeti artar. Burnunu yukarı çektikçe kinetik enerji irtifaya dönüşür; hız `900 units/sec` altına düşerse stall (tutunma kaybı) tetiklenir ve burun otomatik aşağı düşer.
  * Dönüşlerde Roll ve Yaw açıları `RInterpTo` ile yumuşatılarak bank dönüşü hissi verilir.
* **Güneş Enerjisi (Solar Charge) & Güneş Boşlukları (Sun Pockets):**
  * Gökyüzüne dikey atılan periyodik `LineTraceByChannel` açık güneş boşluklarını (Sun Pockets / God-Rays) gördükçe batarya tam hızla dolar (`SolarCharge`: %0'dan %100'e, `%12.5/sn`). Duman tavanının (Smog Canopy) altında kalındığında şarj tamamen kesilir (`%0/sn`).
  * Oyun boyunca şarj hızı kısmen düşürülmez (ya tam şarj vardır ya da gölgedesindir); bacalar açık kaldıkça duman tavanı genişleyerek güneşin ulaşabildiği boşlukları daraltır ve oyuncuyu şarj olmak için kalan güneş koridorlarına yetişmeye zorlar.
* **Solar Boost:**
  * `Space` veya `Shift` basılı tutulduğunda bataryayı hızla harcar.
  * Planörün o anki gövde vektörü (Forward Vector) doğrultusunda ani bir itme kuvveti sağlar (+2200 units/sec).
  * Burnunu yukarı dikip boost basarsan dikine gökyüzüne tırmanabilir; düz uçuşta hedefe doğru roket gibi dalabilirsin.

### 2.2 Camera: Dynamic Decoupled Chase Cam (Ayrık Kamera)
Uçuş yönü ile bakış/nişan yönünü birbirinden tamamen ayıran kamera yapısı:
* **Gövde Bağımsızlığı:** Planör WASD girdileriyle kendi ekseninde uçarken, kamera fare hareketleriyle serbestçe kokpit çevresinde dönebilir (`SpringArm` bağımsız yaw/pitch).
* **Serbest Bakış Açısı (Pitch & Yaw Clamp):** Fare ile sağa, sola, aşağıya ve yukarıya bakılabilir (Gövdeye göre yatayda +/- 85°, dikeyde -60° / +45° sınır).
* **Nişan Koni Sınırı & Manyetik Çekim (Clamped Aiming & Subtle Homing):**
  * **135° Koni Sınırı:** Mermi fırlatma yönü Crosshair'ın dünya izdüşümüne (`DeprojectScreenToWorld` -> `LineTrace`) bakar; uçağın `Forward Vector` açısına göre maksimum 135 derecelik bir koni açısıyla sınırlandırılır. Oyuncu yandan, aşağıdan ve hafif geriden (baca üstünden geçerken) atış yapabilir; yalnızca tam arkadaki 45 derecelik kör bölge kapalıdır. Bu geniş açı yüksek hızlı flyby geçişlerinde kaçan atışları azaltır.
  * **Hafif Manyetik Çekim (Aim Assist):** Yüksek hızda flyby geçişlerinde milimetrik ıskalamaların yaratacağı haksız sürtünmeyi önlemek amacıyla; Crosshair baca ağzına ~10 derecelik açı toleransı içindeyken fırlatılan kapsül, `ProjectileMovementComponent` üzerindeki hafif bir manyetik homing ivmesiyle kavis çizerek doğrudan baca ağzının merkezine oturur.
* **Dinamik FOV ve Juice:**
  * Standart süzülüş: 90° FOV.
  * Dalış ve Solar Boost anı: 96° FOV, ekran kenarlarında hız çizgileri (speedlines).
  * Özel jetlerin altından geçerken: Ekranın üstünde kırmızı ünlem ikazı (`!`), şiddetli türbülans kamera sarsıntısı ve rüzgar uğultusu artışı.

### 2.3 Kontrol Şeması
| Girdi | Eylem | Mekanik İşlev |
| :--- | :--- | :--- |
| **Mouse X / Y** | Serbest Nişan & Bakış | Kamerayı ve ekrandaki nişangahı (Reticle) serbestçe hedefe yöneltir. |
| **S** | Burun Yukarı (Pitch Up) | İrtifa kazanmak için kanat flaplarını yukarı kaldırır (hız tüketir, net tırmanış sağlamaz). |
| **W** | Burun Aşağı (Pitch Down) | Hız kazanmak için dalışa geçer (irtifa kinetik hıza dönüşür). |
| **A / D** | Kanat Yatırma (Roll & Yaw) | Sola ve sağa aerodinamik bank dönüşü yapar. |
| **Space / Shift** | Solar Boost | Güneş bataryasını harcayarak anlık ileri itme kuvveti verir (Net irtifa kazanmanın tek yolu). |
| **Sol Tık (LMB)** | Karbon Mührü Fırlat | Nişangah doğrultusuna, 135° koni ve ~10° manyetik hedef çekimiyle eko-kapsülü fırlatır. |

---

## 3. Sistemik Mekanikler ve Sayısal Denge Tablosu (Atomic Parameters)

### 3.1 Uçuş ve Enerji Matematiği
```text
[Base Velocity]           = 2400 units/sec
[Max Dive Velocity]       = 4600 units/sec
[Stall Velocity]          = 900 units/sec (Kontrol kaybı ve hızlı burun düşmesi)
[Pitch Rate]              = 105 deg/sec
[Roll / Yaw Rate]         = 125 deg/sec

[Solar Charge Rate]       = %12.5 / sec (Güneş altında 8 saniyede tam dolum)
[Solar Boost Drain Rate]  = %25.0 / sec (Dolu batarya aralıksız 4 saniye boost sağlar)
[Solar Boost Thrust]      = +2200 units/sec anlık ivme
```

### 3.2 Karbon Mührü, Nişan Yardımı ve Cooldown
```text
[Ammo Type]               = Sınırsız mühimmat, tek atışlık kapasite
[Cooldown / Reload Time]  = 5.0 saniye (Iska anında geri dönüş manevrası süresiyle senkronize)
[Max Aim Cone Angle]      = 135.0 derece (Gövde Forward Vector merkezli koni clamp)
[Aim Assist Cone]         = ~10.0 derece hedefleme toleransı (Subtle Magnetic Homing)
[Projectile Speed]        = 5000 units/sec (Hafif yerçekimi eğrisi ve hedef çekimli mermi)
[Aim Assist Max Range]    = 12000 units
[Projectile Gravity]      = 0.25 gravity scale, 4.0 saniye ömür
[Hitbox / Collision]      = Baca tepe açıklığında Sphere Collision (Radius: 240 units)
```

### 3.3 İrtifa Katmanları, Zemin Smog Yatağı, Güneş Boşlukları ve Çarpışma Kuralları
*(Not: Aşağıdaki değerler koddaki güncel ve kesinleşmiş değerlerdir.)*
```text
[Minimum Altitude (Floor)]= 200 units (Zemine çarpınca Crash ekranı, ardından tek tuşla restart)
[Smog Bed Ceiling Height] = 400 units (Zemini yutan yoğun sis tabakası ve görsel zemin telgrafı)
[Pull-Up Warning Height]  = 1200 units (Smog yatağına girmeden önce başlayan kırmızı "PULL UP!" HUD ve ses alarmı)
[Target Rim Heights]      = Baca tabanına göre ağız yüksekliği: Normal ~2400u (model 1200u × 2.0 tip çarpanı; yerleşimde ×0.85–1.2 varyasyon) | Çoklu 600u | Yoğun ~2400u (model 1400u × 1.714, Tip A ile aynı boy)
[Player Spawn Altitude]   = 1800 units (Seviye başlangıç süzülüş irtifası)
[Glider Max Ceiling]      = 7800 units (Planörün çıkabileceği maksimum tavan irtifa)
[VIP Jet Flight Altitude] = 8400 units (Planör tavanının üstündeki stratosferik spline hattı)
[VIP Jet Pass]            = 4500u irtifa; planör 6500u'dan itibaren itilir, 3500u'dan yakına giremez; oyun alanında her 1500u'da 2200u yarıçaplı duman bulutu bırakır, bulutlar 5–10 sn altlarındaki güneşi keser
[VIP Jet Timing]          = 2800 units/sec uçuş hızı; haritadaki rotalardan rastgele biri ~10 saniyede bir seçilir (aynı anda tek jet, ilki 6. saniyede), 2.5 saniye önceden `!` uyarısı
[Smog Canopy Altitude]    = 8500 units (Açık bacaların üstündeki duman tavanı; duman sütunları ~7000u yükselip buraya ulaşır; yarıçap 1500u → 9000u genişler; hiç baca kapatılmazsa gökyüzü ~30 saniyede tamamen kapanır ($C_{\min}$ kadar karbonda))
[Sunlight LineTrace Freq] = 0.2 saniyede bir tavana (güneşe) atılan dikey Line Trace
[Hard Countdown Timer]    = YOK (Yapay dijital saat kaldırıldı; süre daralan güneş boşluklarıyla işler)
```

* **Zemin Smog Katmanı (Smog Bed & Visual Ground Telegraph):**
  * Şehir (zemin −1800u, çatılar ~−1050u) zehirli kahverengi bir sis yatağının altında kalır ve yalnızca soluk silüetler olarak seçilir. Sis iki katmandan oluşur:
    * **`SmogBedHeightFog`:** Z = 0'da `Exponential Height Fog` (yoğunluk 0.35, falloff 2.0, maks. opaklık 1.0). Bina seviyesinde yoğun, baca gövdelerinde hafif pus, yukarıda ince.
    * **`SmogBedSurface`:** Z = 380'de `SM_SmogLayer` yüzeyi (×4 ölçek, collision ve gölge kapalı). Işıksız `M_SmogBed` materyali, dünya koordinatlarında iki ölçekte kayan gürültüyle 0.45–0.8 arası değişen opaklık verir, bacalarla kesiştiği yerde yumuşakça kaybolur (depth fade).
  * Planör 1200 birimin altına indiğinde (Smog Bed'e girmeden 800 birim önce) kokpitte kırmızı **"PULL UP!"** HUD uyarısı ve sesli alarm başlar; böylece yüksek hızda dalış yapan oyuncuya tepki süresi tanınır. 400 birimin altına (sisin içine) girildiğinde görüş düşer ve dikey güneş `LineTrace`'i tamamen kesilir.
  * Şehir sokakları ve binaları bu duman yatağının altında kütlesel silüetler olarak gizlenir; gereksiz görsel karmaşa elenir ve oyuncunun dikkati sadece sisi delip yukarı fırlayan bacalara odaklanır.
* **Güneş Boşluklarına Yetişme Mekaniği (Shrinking Sun Pockets — Tüm Oyun Boyunca):**
  * Oyunda şarj hızının yavaşça düşmesi gibi hissedilmesi güç pasif cezalar yoktur; dikey `LineTrace` güneşi gördüğünde batarya her zaman tam hızla (`%12.5/sn`) dolar, duman tavanına (`Smog Canopy`) çarptığında ise şarj tamamen durur.
  * Tüm seviyeler boyunca açık kalan bacalar gökyüzündeki duman örtüsünü genişleterek güneşin yeryüzüne ulaşabildiği açık pencereleri (**Sun Pockets / God-Rays**) adım adım daraltır.
  * Oyuncu bataryasını doldurmak için daralan bu güneş boşluklarına süzülerek yetişmek zorundadır. Güneş boşlukları tamamen kapanırsa veya oyuncu yetişemezse batarya biter; Solar Boost basamayan planör irtifa kaybederek kaçınılmaz biçimde aşağı çöker.
* **Kesin Başarısızlık (Crash) Şartları ve Yeniden Başlama:**
  * Aşağıdaki **3 durum** doğrudan **Crash (Parçalanma)** sayılır. Ekrana çatlak cam vinyeti, ölüm sebebine özel ikon ve `RESTART [R]` butonu gelir; oyuncu butona tıklayarak veya `R` / `Enter` / `Space` ile seviyeyi anında yeniden başlatır:
    1. **Zemin Tabanına Çakılma (`200 units` Kill Floor):** Enerjisiz kalıp veya dik dalıştan çıkamayıp yere çarpma.
    2. **Baca Gövdesine Fiziksel Çarpma:** Teğet geçiş (flyby) sırasında planörün doğrudan fabrika/baca mesh gövdesine çarpması.
    3. **Tip C Yoğun Duman Sütununa (`NS_HeavyToxicSmog`) Girme:** Ağır sanayi bacalarının çıkardığı katran rengi aşırı yoğun ve yüksek basınçlı duman sütununun içine girmek planörü anında boğar ve düşürür (**Crash**).
  * *Not:* Normal bacaların standart duman sütunlarının (`NS_ToxicSmog`) içinden geçmek planörü patlatmaz; görüşü düşürür. Özel jetin bıraktığı duman bulutları ise altlarındaki güneşi 5–10 sn keser.

### 3.4 Kümülatif Karbon Salınımı ve Seviye Derecelendirmesi (Scoring)
* **Kümülatif Karbon Akışı:** Haritadaki her açık baca saniye başına dinamik karbon üretir:
  $$\text{Anlık Karbon Üretimi} = \text{Açık Baca Sayısı} \times C_{\text{oran}} \quad (\text{Baz Değer: } C_{\text{oran}} = 10\text{ kg CO}_2/\text{sn})$$
  Oyuncu bacaları ne kadar hızlı mühürlerse, toplam salınan karbon miktarı o kadar düşük kalır.
* **Teorik Minimum Baz Salınım ($C_{\min}$):**
  Bir oyuncunun haritadaki tüm bacaları en optimize uçuş rotasında, sıfır ıskayla ve sürekli boost kullanarak kapatabileceği teorik minimum karbon salınımı ($C_{\min}$) harita bazında sabittir (Örnek başlangıç referansı: Seviye 1 için $C_{\min} = 4000\text{ kg}$ (geçici; 23 ağızlı yerleşimde optimal tur ~5500 kg, önerilen bantlar 5000–6000 / 6000–7000 / 7000+ kg); Seviye 2 ve 3 değerleri haritalar kurulunca aynı yöntemle hesaplanacak).
* **Yıldız Derecelendirmesi (COP31 Karbon Karnesi):**
  * ⭐⭐⭐ **3 Yıldız (Net-Zero Hero):** $\text{Toplam Karbon} \le C_{\min} \times 1.25$ (Kusursuz rota, sıfır ıskalama).
  * ⭐⭐ **2 Yıldız (Karbon Nötr Geçiş):** $C_{\min} \times 1.25 < \text{Toplam Karbon} \le C_{\min} \times 1.75$ (Kabul edilebilir uçuş, 1-2 ıskalama).
  * ⭐ **1 Yıldız (Yüksek Emisyon):** $\text{Toplam Karbon} > C_{\min} \times 1.75$ (Çokça ıskalama, son anda kurtarılan süzülüş).

---

## 4. Hedef Anatomisi (3 Gerçekçi Fabrika/Baca Türü) ve Dönüşüm Döngüsü

Tüm hedefler **"Do Not Smoke"** temasına uygun gerçekçi sanayi bacalarından oluşur. Bacalar harita üzerinde hem standart yükseklikte (`~1000–1400u`) hem de Seviye 2'den itibaren Smog Bed sınırına yakın **alçak irtifada (`~600u`, "PULL UP!" alarm bölgesinde)** konumlandırılabilir:

* **Tip A — Normal Tekli Fabrika Bacası (`Normal` | `SM_Smokestack_Single`):**
  * Klasik tekli endüstriyel fabrika bacası. Siyahla gri arası koyu duman sütunu (`NS_ToxicSmog`) çıkarır; Tip B de aynı dumanı kullanır.
  * Duman sütununun içinden geçmek ölümcül değildir (görüşü kısıtlar). Doğrudan teğet geçiş (flyby) ve 135° koni nişanıyla vurulur. Gövdesine çarpmak Crash sebebidir.
* **Tip B — Çoklu / İkiz-Üçüz Santral Bacaları (`Çoklu` | `SM_Smokestack_Cluster`):**
  * Büyük termik santrallerdeki gibi aynı tesis bloğu üzerinde yan yana yükselen 2'li veya 3'lü baca kümeleri.
  * **Mekanik Rolü:** Kapsül fırlatıcının **5.0 saniye cooldown** süresi olduğu için oyuncu tek bir geçişte kümedeki tüm bacaları mühürleyemez. İlk bacayı vurduktan sonra hemen yanındaki kardeş bacaların gövdesine çarpmadan keskin bir kaçış manevrası yapmalı ve geniş bir dönüşle (banking loop) kalan bacaları sırayla kapatmalıdır.
* **Tip C — Yoğun Basınçlı Ana Tahliye Bacası (`Yoğun` | `SM_Smokestack_Heavy`):**
  * Ağır metalurji ve petrokimya tesislerinin geniş ağızlı, gökyüzüne simsiyah, katran gibi aşırı yoğun ve yüksek basınçlı duman (`NS_HeavyToxicSmog`) basan dev bacaları.
  * **Mekanik Rolü (Ölümcül Duman Duvarı):** Bu bacadan çıkan yoğun duman sütunu gökyüzünde dikey bir tehlike duvarı oluşturur; duman sütununun içine girmek planörü anında düşürür (**Crash**). Oyuncu bacanın tam üstünden uçamaz; duman sütununa değmeden **yandan/çaprazdan (angled flyby)** yaklaşmalı ve 135° koni sınırını kullanarak kapsülü yandan baca ağzına basket atmalıdır. Baca mühürlendiği an gökyüzündeki bu ölümcül kara duvar ortadan kalkar ve hava koridoru açılır.

### Başarılı Dönüşüm Hazzı (Transformation FX Chain)
1. Kapsül baca ağzına değer (`OnHit`).
2. Metalik, tatmin edici ağır bir kilitlenme sesi çalar (`SFX_Seal_Latch`).
3. Siyah duman sütunu (ve Tip C'deki ölümcül duman duvarı) anında patlayarak kesilir; ağzında sıcak sarı bir ışıltı belirir (`NS_SealSparkle`); bu, güneş paneli modeli gelene kadar panelin parıltısını temsil eder.
4. Tesisin gövdesinde yeşil durum ışıkları / fotovoltaik paneller ışıldar.
5. Gökyüzündeki kara duman tavanında (`Smog Canopy`) o bacanın üstündeki duman bulutu kaybolur ve yeni bir güneş huzmesi (God-Ray / Sun Pocket) açılır.

*Görsel karşılık:* Her açık bacanın duman sütununun tepesinden, uçuş tavanının üstünde (~9700u) karbon arttıkça genişleyen hacimli bir duman bulutu (`NS_SkySmog`) yayılır ve yarıçapı güneş trace'ini kesen görünmez küreyle aynıdır. Baca kapanınca yeni duman çıkmaz, bulut dağılarak güneş boşluğunu açar. Oyun alanı (zemin ile tavan arası) hiçbir zaman pusla kaplanmaz; oyuncu çevresini her zaman net görür. Tavan net biçimde ikiye ayrılır: koyu, opak duman bulutlarıyla kapalı bölgeler ve açık gökyüzü. Bütün level boyunca gökyüzünden hafif, seyrek bir ışık parçacığı yağmuru ("foton yağmuru", `NS_SunMotes`) düşer; duman bulutlarının kapattığı yerlerde bu yağmur kesilir. Oyuncu uzaktaki açıklıkları ışık yağmurundan görüp oraya yetişmeyi hedefler. Karbon biriktikçe güneş en fazla %60'a kadar hafifçe kararır; seviye bitince tekrar aydınlanır.

---

## 5. 3 Seviyeli İlerleme ve Baca Dağılım Matrisi (Level Beat Chart)

Fabrika türleri tek bir seviyeye hapsolmaz; her seviyede bir önceki seviyenin bacaları varlığını sürdürürken, zorluk ve karmaşıklık oranları kademeli olarak kayar:

| Seviye Haritası | Normal Baca (Tip A) | Çoklu Baca (Tip B) | Yoğun Dumanlı Baca (Tip C) | İrtifa & Ek Çevre Baskısı |
| :--- | :---: | :---: | :---: | :--- |
| **`L_Level01_Valley`** *(Onboarding)* | **~%80** | **~%20** | **%0** | Standart yükseklikte bacalar, geniş güneş boşlukları. |
| **`L_Level02_Corridor`** *(Mid-Game)* | **~%40** | **~%50** | **~%10** | **Alçak Bacalar (`~600u`)** + **Tekil Özel Jet & İroni Pankartı (`4500u`)**. |
| **`L_Level03_Basin`** *(Climax)* | **~%20** | **~%30** | **~%50** | Ölümcül yoğun duman duvarları, daralan son güneş cepleri, pankartlı tekil jet geçişleri. |

```text
[L_Level01_Valley] ──► [WBP_COP31_Summary 1] ──► [L_Level02_Corridor] ──► [WBP_COP31_Summary 2] ──► [L_Level03_Basin] ──► [FINAL NET-ZERO]
 (%80 Normal, %20 Çoklu)                          (%40 Normal, %50 Çoklu, %10 Yoğun + Jet)           (%20 Normal, %30 Çoklu, %50 Yoğun + Climax)
```

### Seviye 1: Vadi Atölyeleri (`L_Level01_Valley` — Onboarding & Temel Uçuş)
* **Atmosfer & Güneş:** Geniş güneş boşlukları (Sun Pockets), ferah vadi, sakin rüzgar. Açık bacalar kapatılmadıkça duman tavanı güneş boşluklarını yavaşça daraltır.
* **Baca Dağılımı (%80 Normal / %20 Çoklu / %0 Yoğun):** Çoğunluğu tekli standart fabrika bacalarından oluşur; seviyenin sonuna doğru oyuncu ilk kez **1 adet İkiz (Çoklu) Baca** ile karşılaşarak 5.0 saniyelik cooldown dönüş manevrasını öğrenir.
* **Mekanik Amaç:** Uçuş kontrollerine, güneş boşluklarına girerek şarj olmaya, Solar Boost ile irtifa kazanmaya ve teğet atış disiplinine alışma (18 baca / 23 ağız: 13 Normal, 4 Çoklu, 1 Yoğun test bacası; hepsi ±25000u'luk sınır küplerinin içinde, sınırın 2500u dışında görünmez yumuşak duvar).

### Seviye 2: Lojistik ve Sanayi Koridoru (`L_Level02_Corridor` — Çoklu Kümeler, Alçak İrtifa & Pankartlı Jet)
* **Atmosfer & Güneş:** Parçalı gri duman tavanı, orta ölçekli güneş koridorları. Oyuncu şarj olmak için rotasını güneş pencerelerinden geçecek şekilde planlamalıdır.
* **Baca Dağılımı (%40 Normal / %50 Çoklu / %10 Yoğun) & Yeni Tehditler:**
  * **Çoklu ve Alçak İrtifa Bacaları:** Haritanın yarısı ikili-üçlü baca kümelerinden oluşur ve bazıları Smog Bed (`400u`) sınırının hemen üstüne (`~600u`) yerleştirilmiştir; oyuncu **"PULL UP!"** alarmı çalarken sığ dalışlar yapar.
  * **İlk Yoğun Baca Tanışması (%10):** Seviyenin kilit noktasında **1 adet Tip C Yoğun Dumanlı Baca** yer alır; oyuncu içine girilmez kara duman sütununa yandan yaklaşmayı güvenli bir ortamda ilk kez deneyimler.
  * **Pankartlı Özel Jet (The VIP Polluter & Aerial Banner):** Uçuş bandının tam ortasından (`4500u`) ara sıra geçen **tek bir lüks özel jet**. Ekran üstünde 3 kez yanıp sönen kırmızı ikaz ve motor kükremesiyle uyarır. Görünmez bir itme alanı planörü jetten uzak tutar (çarpışma yok); jet geçtiği yerde arkasında koyu duman bulutları bırakır ve bu bulutlar 5–10 sn boyunca altlarındaki güneşi keser. Jetin kuyruğunda çektiği dev pankartta (`SM_JetBanner`) ikiyüzlü iklim zirvesi ve "greenwashing" sloganları sırayla yer alır:
    * *"ON MY WAY TO CLIMATE SUMMIT!"*
    * *"DON'T FORGET YOUR PAPER STRAW!"*
    * *"THOUGHTS & PRAYERS FOR THE PLANET"*
    * *"NET-ZERO BY 2099!"*

### Seviye 3: Mega Ağır Sanayi Havzası (`L_Level03_Basin` — Yoğun Duman Duvarları & Son Güneş Cepleri)
* **Atmosfer & Güneş:** Kapkara fırtınalı gökyüzü, şimşekler ve hızla daralan son güneş huzmeleri (God-Rays). Oyuncu her atıştan sonra hayatta kalmak için bir sonraki açık güneş cebine (Sun Pocket) yetişmek zorundadır.
* **Baca Dağılımı (%20 Normal / %30 Çoklu / %50 Yoğun) & Climax:**
  * **Ölümcül Duman Koridorları (%50 Yoğun):** Bacaların yarısı gökyüzüne ölümcül siyah duman duvarları (`NS_HeavyToxicSmog`) diken Tip C bacalardır. Oyuncu bu duman sütunlarına çarpmadan aralarından slalom yapmalı ve yandan açılı atışlarla (angled flyby) bacaları tek tek susturarak hava sahasını açmalıdır.
  * **Pankartlı Özel Jet Geçişleri:** Çapraz jet filoları yoktur; bacaların yarattığı yoğunluk ana zorluktur. Ara sıra geçen tekil özel jet, yeni ironik pankart mesajlarıyla gökyüzünden süzülür ve altından geçilirse türbülans yaratır.

---

## 6. Ekip Rolleri, Görev Dağılımı ve Varlık Şartnamesi

### 6.1 Quark — Lead Game Designer & Game Developer
* **Rol & Sorumluluk:** Oyun mimarisi, uçuş kinematikleri, mermi matematiği, hedef durum makineleri, seviye akış sistemi ve teknik entegrasyon.
* **Teslim Edilecek Blueprintler ve Sistemler:**
  * **`BP_GliderPawn`:**
    * `Tick` bazlı aerodinamik süzülüş, pitch/roll girdileri ve `RInterpTo` açı yumuşatması.
    * Net enerji kaybı kuralı: Kinetik dalış tırmanışı asla başlangıç irtifasını aşamaz; hız < 900 olduğunda stall burun düşmesi.
    * Solar Boost mekaniği: Bataryayı tüketerek +2200 hız ivmesi ve net irtifa kazanımının tek kaynağı.
    * Bağımsız `SpringArm` kamera kontrolü (mouse look).
    * Periyodik dikey `LineTrace` ile açık güneş boşlukları (Sun Pockets / God-Rays) algılama ve tam hızlı (`%12.5/sn`) batarya şarj döngüsü.
    * Gövde çarpışma kontrolü: Zemin tabanı (`200u`), baca gövdeleri veya Tip C yoğun duman sütunlarına (`NS_HeavyToxicSmog`) temas anında Crash ve ölüm sebebine göre Crash ekranı (restart butonu) tetikleme.
  * **`BP_CarbonSeal` & Nişan Sistemi:**
    * Kamera crosshair ekran noktasından dünyaya `LineTrace` atımı ve uçağın `Forward Vector` eksenine göre 135 derecelik koni açı sınırlandırması (clamp).
    * ~10 derecelik hedefleme toleransında baca ağzına doğru hafif manyetik homing (Aim Assist) ivmesi.
    * `ProjectileMovementComponent` ile yerçekimli eko-kapsül fırlatma fiziği.
    * 5.0 saniyelik mühür atış cooldown zamanlayıcısı ve mantığı.
  * **`BP_Smokestack` (Normal, Çoklu Küme ve Yoğun Dumanlı Modüler Baca Sınıfı):**
    * Baca tepe açıklığında `Sphere Collision` (Radius: 240 birim) vuruş algılama (`OnHit`) ve gövde mesh'inde fiziksel Crash çarpışması.
    * Tip B (`Çoklu / Clustered`) kurulumu için aynı taban üzerinde 2'li veya 3'lü bağımsız baca ağzı desteği.
    * Tip C (`Yoğun / Heavy`) varyantı için duman sütunu boyunca dikey `Capsule/Cylinder Collision` (`bIsLethalPlume = true`) — planör içine girdiğinde Crash tetikler; baca mühürlendiğinde bu ölümcül collision anında kapatılır.
    * Baca durum makinesi (`bIsSealed`: kara dumanı ve ölümcül duman collider'ını kes, ışıltıyı başlat, metalik kilitlenme sesini tetikle, aktif baca sayısını düşür, gökyüzündeki güneş boşluğunu genişlet).
  * **`BP_PrivateJet` (Tekil Geçiş & İroni Pankartı):**
    * Uçuş bandının ortasındaki (`4500u`) spline rotası üzerinde belirli aralıklarla tekil otonom uçuş; planör yumuşak itme alanıyla jetten uzak tutulur.
    * Kuyruk kısmında yer alan pankart düzleminde (`SM_JetBanner` + `WidgetComponent`) her geçişte sırayla değişen hiciv sloganlarını (*"ON MY WAY TO CLIMATE SUMMIT!"*, *"DON'T FORGET YOUR PAPER STRAW!"*, *"THOUGHTS & PRAYERS FOR THE PLANET"*, *"NET-ZERO BY 2099!"*) gösterme.
    * Yaklaşırken `BP_GliderPawn` ve HUD'a sinyal göndererek ekran üstü kırmızı `!` ikazı ve yaklaşma motor sesini tetikleme.
    * Jet arkasında `NS_SkySmog` duman bulutları; her bulut 5–10 sn altındaki güneşi keser (planörün güneş kontrolü ve HUD'daki SUN oranı bunu hesaba katar).
  * **`BP_GameManager`, Güneş Boşlukları & Seviye Akışı:**
    * Kümülatif Karbon Sayacı: Açık bacalara bağlı dinamik karbon üretimini ($\text{Açık Baca} \times 10\text{ kg/sn}$) Tick bazında toplama.
    * Dinamik Duman Tavanı (`Smog Canopy` / `BP_SunPocket`): Açık bacalar kaldıkça gökyüzündeki güneş pencerelerini daraltma; mühürlenen her bacada güneş koridoru açma.
    * Harita bazlı teorik minimum karbon ($C_{\min}$) ile kıyaslayarak seviye sonu 1-3 yıldız derecesini ve isabet oranını hesaplama.
    * `L_Level01_Valley`, `L_Level02_Corridor`, `L_Level03_Basin` haritalarının yüklenmesi ve `WBP_COP31_Summary` ekranı tetikleyicisi.
  * **Çevre & Tehlike Kurulumu:**
    * Zemin zehirli sis yatağı (`Exponential Height Fog` 0–400 birim) ile görsel zemin telgrafı ve `1200u` altında `"PULL UP!"` irtifa uyarısı tetikleyicisi.
    * `200u` ölüm tabanı (Kill Floor), baca gövdesi ve Tip C yoğun duman sütunu çarpışmalarında Crash ekranı ve tek tuşla yeniden başlama.
* **Girdi Bağımlılıkları:** Barlas'tan `SK_SolarGlider` ve baca mesh'lerini alır; Berf'ten `WBP_FlightHUD`, `WBP_COP31_Summary` ve Sound Cue'ları alır.

---

### 6.2 Berf — UI/UX Designer & Sound Designer
* **Rol & Sorumluluk:** Arayüz tasarımı, oyuncu geri bildirim göstergeleri (HUD), seviye sonu COP31 karbon karnesi, dinamik ses tasarımı ve atmosferik miksaj.
* **Teslim Edilecek UI/UX Widgetları:**
  * **Ana Menü (`UDoNotSmokeMainMenuWidget`):** Berf'in başlangıç görseli (`T_UI_StartValley`) önünde, solda oyun ikonu, "DO NOT SMOKE" başlığı, slogan ve Berf'in panel görselli PLAY / QUIT butonları; sağ altta EN/TR/NL karesi. QUIT onay sorar. Play sonrası kararıp level'e açılır. ESC ile aynı düzende pause menüsü (RESUME / EXIT / dil) açılır.
  * **Dil:** Oyunun varsayılan dili İngilizce; LANGUAGE butonu İngilizce → Türkçe → Hollandaca arasında döner ve seçim kalıcıdır. Tüm ekran metinleri tek bir kod içi tablodan gelir (`DoNotSmokeLocale`).
  * **`WBP_FlightHUD`:**
    * **Merkez:** Dinamik nişangah (`T_UI_Crosshair`) ve etrafında 5.0 saniyelik radyal dolum halkası: koyu zemin (`T_UI_RingDark`), cooldown sırasında dolan beyaz halka (`T_UI_Ring`), atışa hazır olunca yeşil halka (`T_UI_RingGreen`). Nişangah rengi: cooldown'da kırmızı, manyetik kilitte yeşil, koni sınırında sarı.
    * **Sağ Gösterge:** Dikey **Altimeter** cetveli (`T_UI_GaugeAltitudeRuler`) ve üzerinde irtifaya göre hareket eden ok (`T_UI_ArrowLeft`). Cetvelin renk bantları irtifa katmanlarına denk gelir: koyu 0–400u (Smog Bed), kırmızı 400–600u (PULL UP), turuncu 600–1400u (baca bandı), yeşil 1400–7800u (açık gökyüzü). Uyarı artık 1200u'da başlıyor; cetvelin 400–600u kırmızı bandı çizime bağlı olduğu için şimdilik değişmedi.
    * **Sol Gösterge:** Yay şeklinde **Solar Battery** (`T_UI_ArcGauge` çerçeve + alttan yukarı dolan yay): şarjda yeşil (`T_UI_ArcGreen`), boost anında mavi (`T_UI_ArcBlue`), %25'in altında kırmızı (`T_UI_ArcRed`).
    * **Üst Kenar İkazı:** Jet gönderildiği anda üst barın sağında, barla aynı yükseklikte, yanıp sönmeden 3 saniye görünen uyarı paneli (`T_UI_WarningPanel`): sadece "! VIP JET INCOMING !". Slogan yalnızca jetin arkasındaki pankartta görünür.
    * **Tehlike Göstergeleri:** `1200 birimin` altına inildiğinde yanıp sönen yukarı ok (`T_UI_ChevronUp`) ile **"PULL UP!"** paneli ve ekran kenarlarında nabız gibi atan kırmızı vinyet (`T_UI_HazardFrame`).
    * **Tuş Paneli (Sol Üst):** Kalkışta beliren yarı saydam panel: W/S pitch, A/D roll, Mouse nişan, Sol Tık atış, Space/Shift boost ve "güneşte şarj ol" ipucu; 20 sn sonra sönerek kaybolur.
    * **Geri Sayım:** Play'den sonra HUD açılır, dünya donuk kalır ve ortada büyüyerek beliren "3, 2, 1, RENEW IT!" çıkar; uçuş "RENEW IT!" anında başlar. Restart menüyü atlayıp doğrudan geri sayıma gider.
    * **Üst Bar:** İki bölmeli `T_UI_HudBar`. Sol bölmede açık baca sayısı, güneş boşluğu oranı ve anlık yıldız; sağ bölmede duman bulutu ikonu (`T_UI_SmokeCloud`) ile CO2 kg / $C_{\min}$.
  * **COP31 Gazete Ekranı (`UDoNotSmokeSummaryWidget`, Seviye Sonu Manşeti & Karbon Karnesi):**
    * **Gazete Ön Sayfası:** Oyun dünyası durur (planör, jetler, zamanlayıcılar) ve kararır; sayfayla birlikte UI konfetisi patlar (HUD paletinden ~140 parça, ~3.5 sn düşüp kaybolur), gazete sayfası (`T_UI_Newspaper`) iki tur dönüp büyüyerek ekrana gelir ve hafif eğik durur. Başlıkta "Do Not Smoke" logosu (`T_UI_Prohibition` + `T_UI_StackPuff`) ve **THE DAILY SMOG** yazar.
      * **3 Yıldız Manşeti:** *"GÖKYÜZÜ NEFES ALDI: GİZEMLİ PLANÖR SANAYİ BACALARINI KAYNAĞINDA MÜHÜRLEDİ!"*
      * **2 Yıldız Manşeti:** *"EMİSYONLAR FRENLENDİ AMA KARBON BULUTU TAM DAĞILMADI"*
      * **1 Yıldız Manşeti:** *"KIL PAYI KURTULUŞ: BACALAR KAPANDI AMA ŞEHİR İS YUTTU"*
    * **Manşet Fotoğrafı:** Yıldıza göre vadi görseli: 1 yıldız kasvetli (`T_UI_Valley1Star`), 2 yıldız kısmen açılmış (`T_UI_Valley2Star`), 3 yıldız güneşli (`T_UI_ValleyScene`).
    * **AUDIT Kutusu:** Yıldızlar (dolu/boş), duman ikonu ile Toplam Karbon vs. Minimum Hedef ($C_{\min}$), Seviye Süresi (sn) ve Mühür İsabet Oranı (% Accuracy).
    * **Alt Şerit:** `[R] RESTART SECTOR`; `R` / `Enter` / `Space` seviyeyi yeniden başlatır. Bir sonraki seviyeye geçiş, diğer haritalar hazır olunca eklenecek.
  * **Crash Ekranı (`UDoNotSmokeCrashWidget`):**
    * Kaza anında ekranı saran çatlak cam vinyeti (`T_UI_CrackOverlay`).
    * Ölüm sebebine göre ikon: yere çakılma `T_UI_GliderDive`, baca gövdesine çarpma `T_UI_SmokestackBurst`, Tip C yoğun duman sütununda boğulma `T_UI_SmokeWarning`.
    * Klavye tuşu görünümlü restart butonu (`T_UI_Keycap`, "RESTART [R]"); tıklama veya `R` / `Enter` / `Space` ile seviye yeniden yüklenir.
* **Teslim Edilecek Ses Varlıkları (Sound Cues & SFX):**
  * **Uçuş Grubu:** Dinamik hıza bağlı rüzgar uğultusu döngüsü (`Loop_Wind_Whoosh`), Solar Boost plazma itme sesi (`SFX_Solar_Boost`).
  * **Mekanik & Mühür Grubu:** Kapsül fırlatma hava sesi (`SFX_Seal_Fire`), baca kapağı oturma ve kilitlenme çelik sesi (`SFX_Seal_Latch`).
  * **Tehdit & Alarm Grubu:** Özel jet motoru geçiş kükremesi (`SFX_Jet_Roar`), kokpit "PULL UP!" tehlike ikaz tonu (`SFX_Alarm_PullUp`), yoğun basınçlı baca uğultusu (`SFX_Heavy_Smog_Rumble`), türbülans sarsıntı sesi.
  * **Müzik Miksi:** Seviye 1 sakin synthwave -> Seviye 2 tempolu elektronik -> Seviye 3 fırtınalı endüstriyel synth climax.
* **Girdi Bağımlılıkları:** Barlas'tan "Do Not Smoke" logo vektörünü alır; Quark'a UI widgetlarını ve ses varlıklarını entegrasyon için teslim eder.

---

### 6.3 Barlas — 3D Artist
* **Rol & Sorumluluk:** Planör ve jet modelleme, modüler sanayi mimarisi, zemin kiti, marka/logo tasarımı ve Niagara VFX görsel işbirliği.
* **Teslim Edilecek 3D Modeller (Stylized & Low-Poly):**
  * **`SK_SolarGlider`:** Kanatlarında fotovoltaik hücre desenleri olan zarif, hafif, stilize kahraman planör modeli.
  * **`SM_PrivateJet` & `SM_JetBanner`:** Aerodinamik, şık, arkasından yoğun duman bırakan düşük poligonlu özel jet modeli ve kuyruğuna bağlı uçan pankart düzlemi.
  * **`SM_Factory_Kit` (3 Modüler Fabrika/Baca Arketipi — Standart ve Alçak Ölçeklenebilir):**
    * `SM_Smokestack_Single` (Tip A — Normal): Klasik tekli endüstriyel silindirik baca.
    * `SM_Smokestack_Cluster` (Tip B — Çoklu): Aynı santral çatısı üzerinde yan yana yükselen ikili/üçlü baca bloğu.
    * `SM_Smokestack_Heavy` (Tip C — Yoğun): Geniş ağızlı, ağır sanayi yüksek basınçlı ana tahliye bacası.
  * **`SM_City_Blockout`:** Sis tabakasının altına yerleştirilecek hafif, ücretsiz hazır low-poly bina kiti entegrasyonu (Fab / Unreal Marketplace).
  * **`SM_Terrain_Base`:** Düşük poligonlu vadi, plato ve kanyon zemin mesh'leri.
* **Görsel Kimlik & Tanıtım:**
  * **"Do Not Smoke" Logosu:** Kırmızı çember içinde sigara yerine tütmekte olan endüstriyel fabrika bacası ikonu.
  * **Tanıtım Görselleri:** Itch.io kapak afişi (Banner), oyun ikonu ve vitrin görselleri.
* **Niagara VFX Görsel Desteği (Quark ile Ortak):**
  * `NS_ToxicSmog` (standart gri/siyah duman sütunu), `NS_HeavyToxicSmog` (Tip C içine girilmez katran siyahı yoğun basınçlı duman sütunu), `NS_SealSparkle` (kapanan ağızda sıcak sarı ışıltı; eski `NS_CleanSteam` beyaz buharın yerine), `NS_JetWake` (jet hava distorsiyonu), `NS_SunMotes` (açık gökyüzünden yağan ışık çizgileri), `NS_SolarBoost` (itme alevi), `NS_Speedlines` (hız çizgileri).
* **Girdi Bağımlılıkları:** Quark'tan model ölçek ve hedef açıklığı ölçülerini alır; modelleri Quark'a blueprint aktörlerine bağlanması için teslim eder; logoyu Berf'e arayüzde kullanması için verir.

---

### 6.4 Ekip İçi Bağımlılık ve Entegrasyon Akışı

```text
[Barlas: SK_SolarGlider & Modeller] ────► [Quark: BP_GliderPawn & BP_Smokestack] ◄──── [Berf: SFX & Sound Cues]
                                                          │
[Barlas: Do Not Smoke Logosu] ──────────► [Berf: WBP_FlightHUD & Menüler] ──────────► [Quark: HUD Entegrasyonu]
                                                          │
[Barlas: SM_City_Blockout] ─────────────► [Quark: Exponential Fog & Seviye 1-3] ◄─── [Berf: WBP_COP31_Summary]
```

| Kimden | Kime | Teslim Edilen Varlık / Çıktı | Amaç |
| :--- | :--- | :--- | :--- |
| **Barlas** | **Quark** | `SK_SolarGlider`, `SM_Factory_Kit`, `SM_PrivateJet` | Pawn ve hedef aktörlerinin mesh bileşenlerine atanması. |
| **Barlas** | **Berf** | "Do Not Smoke" Logo Vektörü / PNG | HUD açılış ekranı ve menü butonlarında marka kullanımı. |
| **Berf** | **Quark** | `WBP_FlightHUD`, `WBP_COP31_Summary` | Uçuş verilerinin (irtifa, şarj, cooldown) ekrana bağlanması. |
| **Berf** | **Quark** | Sound Cue'lar (Rüzgar, Latch, Roar, Alarm) | Animasyon ve çarpışma event dispatcher'larına ses bağlanması. |
| **Quark** | **Berf & Barlas** | Oynanabilir Seviye Testleri (.umap) | Ses seviyelerinin ve model ölçeklerinin oyunda test edilmesi. |

---

## 7. Game Feel ve Onboarding Prensipleri (Juice Checklist)

1. **Metinsiz Onboarding:** Seviye 1'de yazı basılmaz; ilk baca tam uçuş rotasındadır. İlk isabet 15. saniyede yaşatılır.
2. **Kamera Tekmesi:** Kapsül atışında 0.1s hafif geri tepme sarsıntısı.
3. **Hitstop (Mikro Donma):** Baca vurulduğunda 0.08 saniyelik mikro donma ile vuruş hazzı.
4. **Jet Türbülansı Hissi:** Özel jet geçerken kamerada ani sarsıntı ve uçağın burnunun hafifçe aşağı çekilmesi.

---

## 8. Related Notes

* [`README.md`](README.md)
* [`do-not-smoke-one-pager.md`](do-not-smoke-one-pager.md)
* [`WORKLOG.md`](WORKLOG.md)
* [`TODO.md`](TODO.md)
* [`../../GEMINI.md`](../../GEMINI.md)
