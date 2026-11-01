---
title: "DO NOT SMOKE — One-Pager Game Design Document"
domain: 01-Projects
category: Do-Not-Smoke
status: canonical
tags: [gdd, game-design, jam, monolithic]
---

# DO NOT SMOKE
**One-Pager Game Design Document (GDD)**  
*GameUp Türkiye 48-Hour Game Jam | COP31 Climate Resilience & Action*

---

## 1. High Concept & Metadata

* **Project Title:** DO NOT SMOKE
* **Thematic Categories / Focus:** Karbon Ayakizi ve Bilinçli Tüketim | Yenilenebilir Enerji ve Sürdürülebilirlik
* **Visual Logo / Identity:** Klasik kırmızı "Sigara İçilmez" çemberi; sigara yerine duman tüten fabrika bacası ikonu.
* **Genre:** 3D Arcade Glider & Precision Flyby Targeting (Arcade Uçuş & Hassas Bombardıman)
* **Target Platform:** PC (Windows 64-bit Standalone)
* **Game Engine:** Unreal Engine 5 (AI-First C++ Core + Thin Blueprint Layer)
* **Session Target:** 3 – 5 Minutes (3 Seviyeli yüksek tempolu arcade koşusu)
* **Core Team & Roles:**
  * **Quark:** Lead Game Designer & Game Developer (Pawn Kinematics, Projectile Math, Splines, Level Architecture, Game Flow)
  * **Berf:** UI/UX Designer & Sound Designer (Flight HUD, COP31 Summary Screens, Audio Mix, SFX Suite)
  * **Barlas:** 3D Artist (Hero Glider, 3 Modular Chimneys, Private Jet Mesh, Environment Assets, Logo)
* **Elevator Pitch:** *DO NOT SMOKE*, güneş enerjili bir planörle sanayi dumanlarının ve karbon saçan lüks özel jetlerin arasından süzülerek kirlilik bacalarına teğet geçişlerle (flyby) nokta atışı dalışlar yaptığınız; kirliliğin kaynağını yerinde mühürleyerek kararan gökyüzünü güneşe kavuşturduğunuz bir 3D arcade uçuş oyunudur.

---

## 2. Core Thematic Message & Category Integration (COP31 Focus)

> *"Gökyüzündeki dumanı süpürmek yalnızca semptomu öteler; asıl iklim eylemi, musluğu kaynağında kapatmak ve bacaları 'Do Not Smoke' mührüyle durdurmaktır."*

Oyun iki ana yarışma kategorisini temel oynanış döngüsüne entegre eder:
* **Karbon Ayakizi ve Bilinçli Tüketim:** Emisyonu filtrelemek yerine doğrudan kaynağında (ağır sanayi bacaları) mühürleme, kümülatif karbon salınım skoru ($\text{kg CO}_2$) ve aşırı tüketim simgesi özel jet tehdidi.
* **Yenilenebilir Enerji ve Sürdürülebilirlik:** Fosil yakıtsız **Solar Glider** mekaniği; itme ve tırmanış için gökyüzündeki temiz güneş koridorlarından (Sun Pockets) fotovoltaik enerji toplama zorunluluğu.

---

## 3. Core Gameplay Loop (Oynanış Döngüsü)

```text
[Açık Havada Süzülüş & Güneş Şarjı] 
       │ (Dikey LineTrace ile Güneş Işığında Batarya Dolar: 0% -> 100%)
       ▼
[Kirlilik Bacasını Tespit Et] 
       │ (Yukarı Yükselen Siyah Duman Sütunları)
       ▼
[Teğet Geçiş Manevrası (Flyby) & Bağımsız Nişan] 
       │ (Gövde WASD ile Kinematik Uçar, Kamera Mouse ile 135° Koni Sınırında Nişan Alır)
       ▼
[Karbon Mührünü Ateşle (Drop Seal - LMB)] 
       │ 
       ├─► [İSABET]: Baca Mühürlenir -> Kara Duman Kesilir -> Beyaz Buhar & Güneş Işığı Patlar
       │
       └─► [ISKA]: Kapsül Boşa Gider -> 5s Cooldown -> Geri Dönüş Manevrasında Zaman Kaybedilir
       ▼
[Seviyedeki Tüm Bacaları Mühürle -> COP31 Sonuç Ekranı -> Sonraki Harita (.umap)]
```

---

## 4. The 3Cs (Character, Camera, Controls)

### Character / Vehicle: Solar Glider
* **Fizik (Kinematik Model & Saf Enerji Kaybı):** Motorlu uçak veya ağır Chaos simülasyonu değildir; `Tick` bazlı aerodinamik süzülüş, momentum, hız sönümleme ve yerçekimi dengesiyle hareket eder.
* **Net İrtifa Kaybı Kuralı:** Planör dalıştan kazandığı hızla tırmansa dahi başlangıç irtifasını asla aşamaz (sonsuz süzülüş engellenmiştir). Net irtifa kazanmanın **tek ve mutlak yolu Solar Boost** harcamaktır.
* **Güneş Enerjisi ve Güneş Boşlukları (Solar Charge & Sun Pockets):** Dikey `LineTrace` gökyüzündeki açık güneş boşluklarına (Sun Pockets / God-Rays) baktığı sürece batarya tam hızla (`%12.5/sn`) dolar; duman tavanı altında şarj tamamen kesilir (`%0/sn`). Tüm oyun boyunca bacalar açık kaldıkça duman tavanı genişleyerek güneş boşluklarını daraltır ve oyuncuyu şarj olmak için bu pencerelere yetişmeye zorlar.
* **Solar Boost (Space / Shift):** Basılı tutulduğunda bataryayı harcayarak planörün baktığı doğrultuda (Forward Vector) anlık roket ivmesi sağlar. Tırmanış için zorunludur.
* **İrtifa Katmanları ve Çarpışma Riski (Smog Bed & Crash):** Şehir tabanını örten yoğun zehirli sis tabakası (`Smog Bed`: `0–400 birim`) oyuncu için net bir görsel zemin telgrafıdır. `1200 birimin` altına inildiğinde kokpitte kırmızı **"PULL UP!"** uyarısı ve alarmı başlar; `400 birimin` altında sisin içine girildiğinde güneş ışığı kesilir. Zemin tabanına (`200 birim`), baca gövdelerine veya Tip C bacaların **yoğun basınçlı duman sütununa (`NS_HeavyToxicSmog`)** çarpmak anında **Crash (Instant Restart)** tetikler.

### Camera: Dynamic Decoupled Chase Cam
* Uçağın arkasından takip eder.
* Serbest kamera dönüşü fareye bağlıdır; oyuncu uçağın burnunu çevirmeden kafasını çevirip yanındaki veya altındaki hedeflere bakabilir.
* Hızlandıkça ve dalışa geçtikçe FOV genişler; özel jet yaklaştığında ekran üstünde kırmızı ikaz ünlemi (`!`) belirir ve türbülans sarsıntısı eklenir.

### Controls (Ayrık Uçuş ve Nişan Şeması)
* **Mouse (X / Y):** Bağımsız Kamera ve Nişangah (Crosshair Aiming). Uçağın gövdesini sarsmaz.
* **S:** Burun Yukarı (Pitch Up — Tırmanış, hız tüketir)
* **W:** Burun Aşağı (Pitch Down — Dalış, hız kazandırır)
* **A:** Kanat Sola (Roll & Yaw Left)
* **D:** Kanat Sağa (Roll & Yaw Right)
* **Space / Shift:** Solar Boost (Gövde yönünde roket ivmesi, net irtifa almanın tek yolu)
* **Sol Tık (LMB):** Karbon Mührü Fırlat (Crosshair'ın baktığı doğrultuya doğru 135° koni ve baca ağzına ~10° manyetik hedef çekimiyle eko-kapsülü fırlatır)

---

## 5. Tehdit Dinamikleri, Daralan Güneş Boşlukları ve Karbon Skoru

1. **Iskalamanın Bedeli (Kapsül Cooldown & Zaman Kaybı):**
   * Kapsül fırlatıcı **tek atışlıktır ve 5.0 saniye dolum süresine (cooldown)** sahiptir.
   * Iska anında hedefe tekrar hizalanmak 8–10 saniye manevra gerektirir; bu esnada bacalar durmaksızın karbon salmaya devam eder.
2. **Daralan Güneş Boşlukları (Shrinking Sun Pockets — Tüm Oyun Boyunca):**
   * Ekranda geri sayan yapay bir dijital sayaç veya şarj hızını yavaşlatan pasif cezalar yoktur.
   * Bacalar açık kaldıkça gökyüzündeki duman tavanı genişler ve güneşin ulaşabildiği boşluklar (**Sun Pockets / God-Rays**) daralır; oyuncu tam şarj almak için bu güneş pencerelerine yetişmeye çalışır.
   * Bataryası biten planör irtifa kazanamaz ve süzülüş kaybıyla `400 birimdeki` sis yatağına, ardından `200 birimdeki` zemin tabanına çakılır.
3. **Kümülatif Karbon Salınımı & 3 Yıldız Sistemi:**
   * Açık bacalar açık kaldıkları her an toplam karbon sayacını artırır ($\text{Açık Baca} \times 10\text{ kg/sn}$).
   * Bölüm bazlı teorik minimum karbon ($C_{\min}$) baz alınarak seviye sonu yıldız karnesi verilir:
     * ⭐⭐⭐ **3 Yıldız:** $\le C_{\min} \times 1.25$ (Kusursuz rota, sıfır ıskalama).
     * ⭐⭐ **2 Yıldız:** $C_{\min} \times 1.25 - 1.75$ (Kabul edilebilir uçuş).
     * ⭐ **1 Yıldız:** $> C_{\min} \times 1.75$ (Yüksek kirlilikle kıl payı tamamlama).

---

## 6. 3 Seviyeli İlerleme & Kademeli Baca Dağılımı

Bacalar seviyeye özel kilitlenmez; her seviyede bir önceki seviyenin bacaları korunurken oranlar zorluğa göre kayar:
* **Tip A — Normal Baca (`Normal`):** Tekli fabrika bacası; standart duman sütunu ölümcül değildir.
* **Tip B — Çoklu Santral Bacası (`Çoklu`):** Yan yana ikili/üçlü baca kümeleri; 5.0 sn cooldown yüzünden tek geçişte kapatılamaz, dönüş manevrası gerektirir.
* **Tip C — Yoğun Basınçlı Baca (`Yoğun`):** Gökyüzüne katran siyahı ölümcül bir duman duvarı (`NS_HeavyToxicSmog`) diker; dumanın içine girmek **Crash** sebebidir. Sadece duman sütununa değmeden yandan açılı geçişle (angled flyby) vurulabilir.

| Seviye Haritası | Baca Dağılım Oranı | İrtifa & Çevre Dinamikleri | Bölüm Sonu UI |
| :--- | :--- | :--- | :--- |
| **`L_Level01_Valley`** *(Onboarding)* | **%80 Normal, %20 Çoklu, %0 Yoğun** | Standart yükseklikte bacalar, geniş güneş boşlukları. Uçuşa, şarja ve ilk ikili bacada dönüş manevrasına alışma. | Gazete Manşeti & Karbon Karnesi (1–3 Yıldız) |
| **`L_Level02_Corridor`** *(Mid-Game)* | **%40 Normal, %50 Çoklu, %10 Yoğun** | **Alçak İrtifa Bacaları (`~600u` "PULL UP!" sınırı)** + **Tekil Özel Jet & İroni Pankartı (`4500u`, duman izi güneşi keser)** + İlk Yoğun Baca. | Gazete Manşeti & Karbon Karnesi (1–3 Yıldız) |
| **`L_Level03_Basin`** *(Climax)* | **%20 Normal, %30 Çoklu, %50 Yoğun** | Gökyüzünü bölen ölümcül yoğun duman duvarları, daralan son güneş huzmeleri (God-Rays) ve pankartlı tekil jet geçişleri. | Final Net-Zero Gazete Manşeti |

* **Oyun İçi Hiciv & Mesaj Mekanikleri:**
  * **Özel Jet Arkası İroni Pankartı (`SM_JetBanner`):** Seviye 2 ve 3'te ara sıra geçen tekil özel jetin kuyruğunda *"ON MY WAY TO CLIMATE SUMMIT!"*, *"DON'T FORGET YOUR PAPER STRAW!"*, *"THOUGHTS & PRAYERS FOR THE PLANET"*, *"NET-ZERO BY 2099!"* yazılı dev pankartlar dalgalanır.
  * **Bölüm Sonu Gazete Küpürü (`WBP_COP31_Summary`):** Her seviye sonunda jenerik bir gazete ön sayfası görseli ekrana gelir; alınan 1–3 yıldıza göre gazetenin ana manşeti dinamik değişir.

* **Win Condition:** Seviyedeki tüm bacalar mühürlendiğinde gökyüzündeki dumanlar beyaz su buharına dönüşür, güneş açar ve COP31 sonuç ekranı gelir.
* **Fail Condition (Crash):** Zemin tabanına (`200 birim`), baca gövdelerine veya Tip C bacaların yoğun duman sütununa (`NS_HeavyToxicSmog`) girmek. Ölüm sebebine göre ikonlu Crash ekranı ve tek tuşla yeniden başlama (`R`).

---

## 7. Related Notes

* [`README.md`](README.md)
* [`game-design-document-tr.md`](game-design-document-tr.md)
* [`WORKLOG.md`](WORKLOG.md)
* [`TODO.md`](TODO.md)
* [`../../GEMINI.md`](../../GEMINI.md)
