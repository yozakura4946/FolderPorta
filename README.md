# Folder Portal

A lightweight, zero-install folder shortcut launcher for Windows built with Win32 C++. 
Windows向けの超軽量なフォルダショートカット・ランチャーです。Win32 APIで構築されているため、インストール不要で一瞬で起動します。

---

## English Edition

### Features

- **Language Toggle**: Switch between English and Japanese dynamically in real-time from the drop-down menu. Your preference is automatically saved.
- **Dynamic Buttons**: Automatically generates shortcut buttons based on `folders.txt`.
- **Auto-Resizing Window**: The window dynamically adjusts its height to perfectly fit the number of registered buttons.
- **Zero-Setup (Self-Generating Config)**: If `folders.txt` does not exist, the app automatically creates it with safe default directories.
- **Intuitive Multi-Window Control**:
  - **Left-Click**: Opens the folder in a **new** Explorer window every time.
  - **Right-Click**: Searches for currently open Explorer windows for that path. If found, restores and brings **all** matching windows to the front. If not found, launches a new one.

### How to Use

1. Run `FolderPortal.exe`. (A default `folders.txt` will be automatically generated in the same folder).
2. Select your preferred language (Japanese or English) from the drop-down menu at the top. The UI translates instantly.
3. Click the "Settings" button to edit the configuration in Notepad.
   - Format: `DisplayName,C:\Path\To\Folder`
   - Your language preference is also saved here as `Language,English` or `Language,Japanese`.
4. Relaunch the app to apply your folder changes.

### Build Instructions

#### Using Visual Studio (MSVC)
1. Create a C++ "Windows Desktop Wizard" project and select the "Empty Project" option.
2. Go to **Project Properties -> Linker -> System -> SubSystem** and change it to `Windows (/SUBSYSTEM:WINDOWS)`.
3. Add `main.cpp` to your source files and build/run.

#### Using Command Line (g++)
If you have a MinGW environment (like w64devkit), you can compile it with:
```bash
g++ -O3 -mwindows main.cpp -o FolderPortal.exe -lshlwapi -lole32 -loleaut32
```

---

## 日本語版

### 主な機能

- **表示言語の切り替え**: 画面上部のドロップダウンメニューから、日本語と英語をリアルタイムに切り替えられます。設定は自動で保存されます。
- **動的なボタン生成**: `folders.txt` に登録したフォルダの数に合わせて、ボタンが自動で並びます。
- **ウィンドウサイズの自動調整**: 登録数に合わせて、アプリの窓枠の大きさがぴったりに自動調整されます。
- **日本語（UTF-8）に完全対応**: メモ帳で簡単に日本語の表示名を編集できます。
- **直感的なマルチウィンドウ対応**: 
  - **左クリック**: 毎回新しくそのフォルダのウィンドウを開きます。
  - **右クリック**: すでに同じフォルダが開いている場合、開いているすべてのウィンドウを最前面に呼び出してアクティブにします（開いていない場合は新規起動します）。

### 使い方

1. アプリを起動すると、同じフォルダに自動的に `folders.txt` が生成されます。
2. 画面上部のメニューから表示言語（日本語 / English）を選択します。画面全体の表示が即座に切り替わります。
3. 「設定を開く」ボタンから、メモ帳で `folders.txt` を編集します。
   - 書式: `表示名,フォルダの絶対パス`
   - 選択した言語設定も、このファイル内に `Language,Japanese` のように自動保存されます。
4. アプリを再起動すると、追加・変更したフォルダショートカットが反映されます。

### ビルド方法

#### Visual Studio を使用する場合
1. Visual Studio を開き、「Windows デスクトップ ウィザード」から「空のプロジェクト」を作成します。
2. プロジェクトのプロパティから、**リンカー ➡️ システム ➡️ サブシステム** を「Windows (/SUBSYSTEM:WINDOWS)」に変更します。
3. `main.cpp` をプロジェクトに追加し、ビルド（実行）します。

#### コマンドライン（g++）でビルドする場合
`w64devkit` などのMinGW環境がある場合、以下のコマンドでコンパイルできます：
```bash
g++ -O3 -mwindows main.cpp -o FolderPortal.exe -lshlwapi -lole32 -loleaut32
```

---

## License

This project is licensed under the MIT License - see the LICENSE file for details.
