# mikanOS環境差異対応：ELFローダの実装について

## 1. 背景：なぜ本の通り（単純コピー）では動かなかったのか？

### 環境の違い
* **mikanOS本:** 古いリンカ設定（または特定バージョン）を想定。
    * 「ファイル内の配置」と「メモリ上の配置」がほぼ一致していた。
* **今回の環境:** 最新の `ld.lld` (LLVM 14以降など) を使用。
    * セキュリティや最適化のため、セグメント（コードやデータ）の間に**「パディング（隙間）」**を空けて配置するのがデフォルト。

### 発生した問題
本の通りに `kernel_file->Read` でファイルをメモリへ丸ごとコピーした結果、以下のズレが生じた。

1.  **ファイルの中身:** ヘッダの後ろにすぐコードが詰まっている。
2.  **リンカの想定:** 「コードは `0x1000` バイトくらい後ろ（隙間の先）にあるはずだ」としてエントリーポイントを設定。
3.  **結果:** CPUがエントリーポイント（正しい予定地）にジャンプしたが、**単純コピーではまだデータが届いておらず、虚無（ゼロ）を実行してフリーズした。**

## 2. 図解：単純コピー vs ELFロード

```mermaid
graph TD
    subgraph File["カーネルファイル(kernel.elf)"]
        FH[ELFヘッダ]
        FC[実行コード]
    end

    subgraph MemoryFail["失敗：単純コピーの場合"]
        M1[0x100000: ELFヘッダ]
        M2[0x100040: 実行コード(ここに来てしまう)]
        M3[0x101000: 虚無(00 00...)]
        Bug[ERROR: CPUはここ(0x101000)に飛ぶ！]
        M3 --- Bug
    end

    subgraph MemorySuccess["成功：ELFローダの場合"]
        M4[0x100000: (ここは使わない)]
        Gap[隙間(パディング)]
        M5[0x101000: 実行コード(正しい位置に配置)]
        OK[SUCCESS: CPUはここへ飛ぶ]
        M5 --- OK
    end

    File -- 丸ごとコピー --> MemoryFail
    File -- "ヘッダを見て配置(CopyMem)" --> MemorySuccess
````

## 3\. 実装した解決策：簡易ELFローダ

ファイルを「丸ごとメモリ配置」するのではなく、**「一度バッファに読み込んでから、設計図（プログラムヘッダ）に従って正しい位置に配り直す」** 処理を実装した。

### 主要なコードの解説

#### 手順1. 一時バッファへの読み込み

まず、ファイルを解析するために、`AllocatePool` で確保した一時的なメモリ領域にファイル全体を読み込む。これは「解凍前のアーカイブ」を持っている状態に近い。

```c
// 一時バッファにファイルを丸ごと読む
status = gBS->AllocatePool(EfiLoaderData, kernel_file_size, &kernel_buffer);
kernel_file->Read(kernel_file, &kernel_file_size, kernel_buffer);
```

#### 手順2. メモリ領域の計算と確保

ELFヘッダ (`Elf64_Ehdr`) とプログラムヘッダ (`Elf64_Phdr`) を解析し、カーネルが最終的に必要とするメモリ範囲（開始アドレス〜終了アドレス）を計算して `AllocatePages` で確保する。

```c
// PT_LOAD（ロードが必要なセグメント）の範囲を調べて確保する
for (UINTN i = 0; i < ehdr->e_phnum; ++i) {
    if (phdr[i].p_type != PT_LOAD) continue;
    // 開始アドレスの更新
    if (kernel_first_addr > phdr[i].p_vaddr) {
         kernel_first_addr = phdr[i].p_vaddr;
    }
    // 終了アドレスの更新
    if (kernel_last_addr < phdr[i].p_vaddr + phdr[i].p_memsz) {
        kernel_last_addr = phdr[i].p_vaddr + phdr[i].p_memsz;
    }
}
// 必要なページ数を計算して確保
UINTN num_pages = (kernel_last_addr - kernel_first_addr + 0xfff) / 0x1000;
gBS->AllocatePages(AllocateAddress, EfiLoaderData, num_pages, &kernel_first_addr);
```

#### 手順3. セグメントの展開（コピー）

ここが核心部分。ファイル内のオフセット位置 (`p_offset`) にあるデータを、メモリ上の仮想アドレス (`p_vaddr`) にコピーする。これにより、リンカが想定した通りの「隙間」が再現される。

```c
for (UINTN i = 0; i < ehdr->e_phnum; ++i) {
    if (phdr[i].p_type == PT_LOAD) {
        // ファイルの中身をメモリの正しい位置にコピー
        gBS->CopyMem(
            (VOID*)phdr[i].p_vaddr, 
            (VOID*)((UINT64)kernel_buffer + phdr[i].p_offset), 
            phdr[i].p_filesz
        );
        // ファイルサイズよりメモリサイズが大きい場合（BSSなど）、残りを0で埋める
        UINTN remain_bytes = phdr[i].p_memsz - phdr[i].p_filesz;
        gBS->SetMem((VOID*)(phdr[i].p_vaddr + phdr[i].p_filesz), remain_bytes, 0);
    }
}
```

#### 手順4. エントリーポイントの取得

ファイル上の固定位置（`+24`バイト目など）を信じるのではなく、ヘッダに記録されている正しいエントリーポイントを取得する。

```c
UINT64 entry_addr = ehdr->e_entry;
```

## 4\. 結論

この実装により、コンパイラやリンカのバージョンが変わってメモリ配置（アライメント）が変更されても、ヘッダ情報を元に自動的に正しくロードできる**堅牢なブートローダ**になった。
