# Fruit Ninja - משחק בשליטה באמצעות היד

משחק Fruit Ninja למחשב שולחני. שרת Python משתמש במצלמה וב־MediaPipe כדי לזהות את קצה האצבע המורה, ושולח את הקואורדינטות דרך UDP למשחק C++ שמרונדר באמצעות raylib.

## דרישות

- Windows
- CMake 3.20 ומעלה
- Visual Studio עם כלי הפיתוח ל־C++
- Python 3 וספריות `mediapipe` ו־`opencv-python`
- חיבור לאינטרנט בבנייה הראשונה, לצורך הורדת raylib באמצעות CMake

## בנייה

פתחו PowerShell בתיקיית הפרויקט והריצו:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

קובץ ההרצה ייווצר בדרך כלל בנתיב:

```text
build/Release/cam-desktop-fruit-ninja.exe
```

## התקנת רכיבי שרת המצלמה

מומלץ ליצור סביבה וירטואלית מקומית ולהתקין בה את התלויות:

```powershell
py -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install mediapipe opencv-python
```

מודל MediaPipe נמצא ב־`vision/hand_landmarker.task` ונדרש להפעלת שרת המצלמה.

## הפעלה

יש להפעיל את המשחק ואת שרת המצלמה בשני חלונות PowerShell נפרדים.

בחלון הראשון, מתיקיית הפרויקט:

```powershell
.\build\Release\cam-desktop-fruit-ninja.exe
```

בחלון השני:

```powershell
Set-Location .\vision
python .\vision_server.py
```

שרת המצלמה שולח את קואורדינטות האצבע ל־`127.0.0.1:5005`. יש לאפשר גישה למצלמה. לחצו `q` בחלון הווידאו כדי לסגור את שרת המצלמה.

## משחק

- הזיזו את האצבע המורה כדי לשלוט בלהב.
- העבירו את הלהב במהירות דרך פירות כדי לחתוך אותם ולצבור נקודות.
- הימנעו מחיתוך פצצות ומפספוס פירות. המשחק מסתיים לאחר ארבע פסילות.
- במסך סיום המשחק לחצו `R` או הניפו את הלהב במרכז המסך כדי להתחיל מחדש.

## מבנה הפרויקט

```text
include/                 כותרות C++ וסוגי נתונים
src/                     מימוש המשחק, הקליטה מהרשת והמערכות
vision/                  שרת MediaPipe ומודל זיהוי היד
CMakeLists.txt           הגדרות הבנייה
```
