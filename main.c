#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

int main(void) {
    int num;
    // Windowsのコンソール色コード（背景色 + 文字色）
    // 0 = 黒, A = 明るい緑, B = 水色, C = 赤, D = 紫, E = 黄色, F = 白
    char *rainbow_colors[] = {
        "color 0C", // 赤
        "color 0E", // 黄
        "color 0A", // 緑
        "color 0B", // 水色
        "color 01", // 青
        "color 0D", // 紫
        "color 0F"  // 白
    };
    int color_count = sizeof(rainbow_colors) / sizeof(rainbow_colors[0]);

    printf("これは訓練です\n繰り返します\nこれは訓練です\n\n");

    Sleep(2000);

    printf("\n\nエラーが発生しました。\n修復を試みます....\n\n");
    system("PAUSE");

    printf("There are any errors.\nSo, I will restart for me.\n\n");

    for (num = 0; num < 20; num++) {
        printf("error error error\n");
        system(rainbow_colors[num % color_count]); // 色を順番に変更
        Sleep(100); // 色の変化が見えるように少しウェイト
    }

    // 注意: 実行するとPCがシャットダウンします
    system("shutdown /s /t 20");

    while (1) {
        printf("error error error\n");
        system(rainbow_colors[num % color_count]);
        num++;
        Sleep(100);
    }

    return 0;
}