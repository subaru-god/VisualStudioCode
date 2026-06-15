#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <string>
#include <cstdio>
#include <cstring>

using namespace std;

// シンプルに保った Othello 実装（Borland bcc32 互換を意識）
const int BOARD_SIZE = 8;
const int CELL_SIZE = 50;
const int EMPTY = 0;
const int BLACK = 1;
const int WHITE = -1;

const int dr[] = {-1, -1, -1,  0, 0,  1, 1, 1};
const int dc[] = {-1,  0,  1, -1, 1, -1, 0, 1};

struct Move { int r, c; };

class Othello {
public:
    int board[BOARD_SIZE][BOARD_SIZE];
    int turn;
    Othello() { reset(); }
    void reset();
    bool isValidMove(int r, int c, bool flip = false);
    vector<Move> getValidMoves();
};

void Othello::reset() {
    int i, j;
    for (i = 0; i < BOARD_SIZE; ++i) for (j = 0; j < BOARD_SIZE; ++j) board[i][j] = EMPTY;
    board[3][3] = WHITE; board[3][4] = BLACK;
    board[4][3] = BLACK; board[4][4] = WHITE;
    turn = BLACK;
}

bool Othello::isValidMove(int r, int c, bool flip) {
    if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE) return false;
    if (board[r][c] != EMPTY) return false;
    bool valid = false;
    int d;
    for (d = 0; d < 8; ++d) {
        int nr = r + dr[d];
        int nc = c + dc[d];
        int count = 0;
        while (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE && board[nr][nc] == -turn) {
            nr += dr[d]; nc += dc[d]; count++;
        }
        if (count > 0 && nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE && board[nr][nc] == turn) {
            valid = true;
            if (flip) {
                int fr = r + dr[d], fc = c + dc[d];
                while (fr != nr || fc != nc) {
                    board[fr][fc] = turn;
                    fr += dr[d]; fc += dc[d];
                }
            }
        }
    }
    if (valid && flip) board[r][c] = turn;
    return valid;
}

vector<Move> Othello::getValidMoves() {
    vector<Move> moves;
    int i, j;
    for (i = 0; i < BOARD_SIZE; ++i) for (j = 0; j < BOARD_SIZE; ++j) if (isValidMove(i, j)) { Move m; m.r = i; m.c = j; moves.push_back(m); }
    return moves;
}

// グローバル（最小限に）
Othello g_game;
int g_mode = 1; // 0:PvP, 1:PvE, 2:EvE
bool g_gameOver = false;
int g_passCount = 0;
bool g_isPassing = false;
// AI thinking control
bool g_aiThinking = false;
DWORD g_aiThinkDeadline = 0; // GetTickCount() deadline in ms
Move g_selectedAIMove = {0,0};

const RECT BTN_PVP = {430,180,520,210};
const RECT BTN_PVE = {530,180,620,210};
const RECT BTN_EVE = {630,180,720,210};

// 軽量な学習データ（互換性を保持）
const char* DATA_FILE = "othello_dataset.dat";
const unsigned char MOVE_PASS = 0xFF;

// シンプルなランダムAI（改善点: 重みやレベルを追加可能）
Move selectRandomMove(const vector<Move>& moves) {
    Move m = moves[rand() % moves.size()];
    return m;
}

// Count flips that would occur for a move without modifying the real board
int countFlipsForMove(const Othello& game, int r, int c) {
    if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE) return 0;
    if (game.board[r][c] != EMPTY) return 0;
    int total = 0;
    int d;
    for (d = 0; d < 8; ++d) {
        int nr = r + dr[d], nc = c + dc[d];
        int cnt = 0;
        while (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE && game.board[nr][nc] == -game.turn) {
            nr += dr[d]; nc += dc[d]; ++cnt;
        }
        if (cnt > 0 && nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE && game.board[nr][nc] == game.turn) {
            total += cnt;
        }
    }
    return total;
}

// Choose best move by simple heuristic (flip count + corner bonus)
Move selectBestMove(const vector<Move>& moves) {
    if (moves.empty()) { Move m = {0,0}; return m; }
    int bestIdx = 0;
    int bestScore = -1000000;
    for (size_t i = 0; i < moves.size(); ++i) {
        int r = moves[i].r, c = moves[i].c;
        int score = countFlipsForMove(g_game, r, c);
        // corner bonus
        if ((r == 0 && c == 0) || (r == 0 && c == 7) || (r == 7 && c == 0) || (r == 7 && c == 7)) score += 100;
        // edge bonus
        if (r == 0 || r == 7 || c == 0 || c == 7) score += 5;
        if (score > bestScore) { bestScore = score; bestIdx = (int)i; }
    }
    return moves[bestIdx];
}

void saveStep(const Move& mv, bool pass) {
    ofstream ofs(DATA_FILE, ios::app | ios::binary);
    if (!ofs) return;
    if (pass) {
        unsigned char code = MOVE_PASS; ofs.write((char*)&code, 1);
    } else {
        unsigned char code = (unsigned char)((mv.r << 4) | (mv.c & 0x0F)); ofs.write((char*)&code, 1);
    }
}

bool processSingleMove() {
    if (g_gameOver) return false;
    vector<Move> moves = g_game.getValidMoves();
    if (moves.empty()) {
        g_passCount++;
        Move dummy = {0,0}; saveStep(dummy, true);
        if (g_passCount >= 2) { g_gameOver = true; saveStep(dummy, false); return false; }
        g_isPassing = true; g_game.turn = -g_game.turn; return true;
    }
    g_passCount = 0; g_isPassing = false;
    Move mv = selectRandomMove(moves);
    saveStep(mv, false);
    g_game.isValidMove(mv.r, mv.c, true);
    g_game.turn = -g_game.turn;
    return true;
}

// 描画
void DrawButton(HDC mem, const RECT* rc, const char* label, bool active, HFONT hFont) {
    HBRUSH btnBrush = CreateSolidBrush(active ? RGB(150,200,255) : RGB(240,240,240));
    HBRUSH old = (HBRUSH)SelectObject(mem, btnBrush);
    RoundRect(mem, rc->left, rc->top, rc->right, rc->bottom, 8, 8);
    SetBkMode(mem, TRANSPARENT);
    HFONT oldF = (HFONT)SelectObject(mem, hFont);
    int x = rc->left + 12, y = rc->top + 6;
    TextOut(mem, x, y, label, (int)strlen(label));
    SelectObject(mem, oldF);
    SelectObject(mem, old);
    DeleteObject(btnBrush);
}

void DrawBoard(HDC hdc, HWND hwnd) {
    RECT client; GetClientRect(hwnd, &client);
    int winW = client.right - client.left; int winH = client.bottom - client.top;
    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, winW, winH);
    HBITMAP oldBmp = (HBITMAP)SelectObject(mem, bmp);

    HBRUSH bg = CreateSolidBrush(RGB(245,245,245));
    FillRect(mem, &client, bg); DeleteObject(bg);

    // board background
    HBRUSH boardBrush = CreateSolidBrush(RGB(34,139,34));
    HBRUSH oldBrush = (HBRUSH)SelectObject(mem, boardBrush);
    Rectangle(mem, 10, 10, 10 + BOARD_SIZE*CELL_SIZE, 10 + BOARD_SIZE*CELL_SIZE);

    int i;
    for (i = 0; i <= BOARD_SIZE; ++i) {
        MoveToEx(mem, 10 + i*CELL_SIZE, 10, NULL); LineTo(mem, 10 + i*CELL_SIZE, 10 + BOARD_SIZE*CELL_SIZE);
        MoveToEx(mem, 10, 10 + i*CELL_SIZE, NULL); LineTo(mem, 10 + BOARD_SIZE*CELL_SIZE, 10 + i*CELL_SIZE);
    }

    HBRUSH black = CreateSolidBrush(RGB(0,0,0));
    HBRUSH white = CreateSolidBrush(RGB(255,255,255));
    HPEN nullPen = CreatePen(PS_NULL, 0, RGB(0,0,0));
    HPEN oldPen = (HPEN)SelectObject(mem, nullPen);

    int r, c;
    for (r = 0; r < BOARD_SIZE; ++r) for (c = 0; c < BOARD_SIZE; ++c) {
        int left = 10 + c*CELL_SIZE, top = 10 + r*CELL_SIZE;
        if (g_game.board[r][c] == BLACK) { SelectObject(mem, black); Ellipse(mem, left+6, top+6, left+CELL_SIZE-6, top+CELL_SIZE-6); }
        else if (g_game.board[r][c] == WHITE) { SelectObject(mem, white); Ellipse(mem, left+6, top+6, left+CELL_SIZE-6, top+CELL_SIZE-6); }
    }

    // highlight valid moves
    if (!g_gameOver && (g_mode == 0 || (g_mode == 1 && g_game.turn == BLACK))) {
        HBRUSH guide = CreateSolidBrush(RGB(255,200,60)); SelectObject(mem, guide);
        vector<Move> moves = g_game.getValidMoves();
        for (size_t k = 0; k < moves.size(); ++k) {
            int left = 10 + moves[k].c*CELL_SIZE, top = 10 + moves[k].r*CELL_SIZE;
            Ellipse(mem, left+18, top+18, left+CELL_SIZE-18, top+CELL_SIZE-18);
        }
        DeleteObject(guide);
    }

    SelectObject(mem, oldBrush); SelectObject(mem, oldPen);
    DeleteObject(boardBrush); DeleteObject(black); DeleteObject(white); DeleteObject(nullPen);

    // UI font
    HFONT hFont = CreateFontA(16,0,0,0,FW_NORMAL,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");

    // Buttons
    DrawButton(mem, &BTN_PVP, "Human vs Human", g_mode == 0, hFont);
    DrawButton(mem, &BTN_PVE, "Human vs AI", g_mode == 1, hFont);
    DrawButton(mem, &BTN_EVE, "AI vs AI", g_mode == 2, hFont);

    // status panel
    RECT statusRc = { 420, 10, 740, 150 };
    HBRUSH statusBg = CreateSolidBrush(RGB(250,250,250));
    FillRect(mem, &statusRc, statusBg);
    DeleteObject(statusBg);

    SetBkMode(mem, TRANSPARENT);
    HFONT oldFont = (HFONT)SelectObject(mem, hFont);
    char buf[256];
    int blackCount = 0, whiteCount = 0;
    for (r = 0; r < BOARD_SIZE; ++r) for (c = 0; c < BOARD_SIZE; ++c) { if (g_game.board[r][c] == BLACK) ++blackCount; if (g_game.board[r][c] == WHITE) ++whiteCount; }

    TextOut(mem, 430, 18, (g_mode==0)?"Mode: Human vs Human":(g_mode==1)?"Mode: Human vs AI":"Mode: AI vs AI", (g_mode==1)?17: (g_mode==0)?21:9);
    sprintf(buf, "Black: %d", blackCount); TextOut(mem, 430, 46, buf, (int)strlen(buf));
    sprintf(buf, "White: %d", whiteCount); TextOut(mem, 530, 46, buf, (int)strlen(buf));

    if (g_isPassing) TextOut(mem, 430, 80, "PASS: No legal moves", 20);
    else if (g_gameOver) TextOut(mem, 430, 80, "Game Over - press R to restart", 28);
    else {
        sprintf(buf, "Current turn: %s", (g_game.turn == BLACK ? "Black" : "White")); TextOut(mem, 430, 80, buf, (int)strlen(buf));
    }

    // AI thinking countdown
    if (g_aiThinking) {
        DWORD now = GetTickCount();
        int sec = (int)((g_aiThinkDeadline > now) ? ((g_aiThinkDeadline - now + 999) / 1000) : 0);
        sprintf(buf, "AI thinking: %d s", sec);
        TextOut(mem, 430, 110, buf, (int)strlen(buf));
    }

    SelectObject(mem, oldFont);
    DeleteObject(hFont);

    BitBlt(hdc, 0, 0, winW, winH, mem, 0, 0, SRCCOPY);

    SelectObject(mem, oldBmp); DeleteObject(bmp); DeleteDC(mem);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        srand((unsigned)time(NULL));
        g_game.reset(); g_mode = 1;
        SetTimer(hwnd, 1, 300, NULL);
        break;
    case WM_TIMER:
        if (!g_gameOver && (g_mode == 2 || (g_mode == 1 && g_game.turn == WHITE))) {
            // AI thinking flow: give AI up to 10 seconds to "think" before applying selected move
            if (!g_aiThinking) {
                vector<Move> moves = g_game.getValidMoves();
                if (moves.empty()) {
                    // pass
                    g_passCount++;
                    Move dummy = {0,0}; saveStep(dummy, true);
                    if (g_passCount >= 2) { g_gameOver = true; saveStep(dummy, false); }
                    else { g_isPassing = true; g_game.turn = -g_game.turn; }
                } else {
                    // select best move immediately, but apply after think deadline
                    g_selectedAIMove = selectBestMove(moves);
                    g_aiThinkDeadline = GetTickCount() + 10000; // 10 seconds
                    g_aiThinking = true;
                }
            } else {
                if (GetTickCount() >= g_aiThinkDeadline) {
                    // apply selected move
                    saveStep(g_selectedAIMove, false);
                    g_game.isValidMove(g_selectedAIMove.r, g_selectedAIMove.c, true);
                    g_game.turn = -g_game.turn;
                    g_aiThinking = false;
                }
            }
            InvalidateRect(hwnd, NULL, FALSE);
        }
        break;
    case WM_KEYDOWN:
        if (wp == 'R' || wp == 'r') { g_game.reset(); g_gameOver = false; g_passCount = 0; g_isPassing = false; g_aiThinking = false; InvalidateRect(hwnd, NULL, TRUE); }
        break;
    case WM_LBUTTONDOWN: {
        int x = LOWORD(lp), y = HIWORD(lp);
        if (x >= BTN_PVP.left && x <= BTN_PVP.right && y >= BTN_PVP.top && y <= BTN_PVP.bottom) { g_mode = 0; g_aiThinking = false; InvalidateRect(hwnd, NULL, TRUE); break; }
        if (x >= BTN_PVE.left && x <= BTN_PVE.right && y >= BTN_PVE.top && y <= BTN_PVE.bottom) { g_mode = 1; g_aiThinking = false; InvalidateRect(hwnd, NULL, TRUE); break; }
        if (x >= BTN_EVE.left && x <= BTN_EVE.right && y >= BTN_EVE.top && y <= BTN_EVE.bottom) { g_mode = 2; g_aiThinking = false; InvalidateRect(hwnd, NULL, TRUE); break; }

        if (!g_gameOver && (g_mode == 0 || (g_mode == 1 && g_game.turn == BLACK))) {
            int c = x / CELL_SIZE, r = y / CELL_SIZE;
            if (g_game.isValidMove(r, c, true)) {
                Move mv; mv.r = r; mv.c = c; saveStep(mv, false);
                g_game.turn = -g_game.turn; g_passCount = 0; g_isPassing = false;
                InvalidateRect(hwnd, NULL, FALSE); UpdateWindow(hwnd);
            }
        }
        break; }
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps); DrawBoard(hdc, hwnd); EndPaint(hwnd, &ps); break;
    }
    case WM_ERASEBKGND: return 1;
    case WM_DESTROY: KillTimer(hwnd, 1); PostQuitMessage(0); break;
    default: return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    (void)hPrev; (void)lpCmd;
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, hInst, NULL, LoadCursor(NULL, IDC_ARROW), NULL, NULL, "OthelloAI_v2", NULL };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindow("OthelloAI_v2", "Othello - simplified GUI (bcc32)", WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 760, 440, NULL, NULL, hInst, NULL);
    ShowWindow(hwnd, nShow); UpdateWindow(hwnd);
    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    UnregisterClass("OthelloAI_v2", wc.hInstance);
    return (int)msg.wParam;
}
