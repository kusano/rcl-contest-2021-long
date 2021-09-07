//  g++ -O2 -o A A.cpp && ./A < tester/input_0.txt > tester/output_0.txt

#include <iostream>
#include <vector>
#include <cassert>
#include <chrono>
#include <cmath>
using namespace std;

const int N = 16;
const int M = 5000;
const int T = 1000;

struct RC
{
    int r, c;
    RC(int r, int c): r(r), c(c) {}
};

struct Move
{
    //  購入はrc1.r=-1
    //  パスはrc2.r=-1
    RC rc1, rc2;
    Move(RC rc1, RC rc2): rc1(rc1), rc2(rc2) {}
};

vector<RC> SRC[T];
vector<int> SV[T];
vector<RC> ERC[T];

int xor64(void) {
    static uint64_t x = 88172645463325252ULL;
    x ^= x<<13;
    x ^= x>> 7;
    x ^= x<<17;
    return int(x&0x7fffffff);
}

int calc_k_sub(bool machine[N][N], int r, int c, vector<RC> *V)
{
    assert(machine[r][c]);

    machine[r][c] = false;
    V->push_back(RC(r, c));

    static int dr[] = {-1, 1, 0, 0};
    static int dc[] = {0, 0, -1, 1};
    int n = 1;
    for (int d=0; d<4; d++)
    {
        int tr = r + dr[d];
        int tc = c + dc[d];
        if (0<=tr && tr<N &&
            0<=tc && tc<N &&
            machine[tr][tc])
            n += calc_k_sub(machine, tr, tc, V);
    }
    return n;
}

int calc_k(bool machine[N][N], int r, int c)
{
    assert(machine[r][c]);

    static vector<RC> V;
    int n = calc_k_sub(machine, r, c, &V);

    while (!V.empty())
    {
        machine[V.back().r][V.back().c] = true;
        V.pop_back();
    }

    return n;
}

long long calc_score(const vector<Move> &moves)
{
    int vege[N][N] = {};
    bool machine[N][N] = {};
    long long money = 1;
    int j = 0;

    for (int t=0; t<T; t++)
    {
        const RC &rc1 = moves[t].rc1;
        const RC &rc2 = moves[t].rc2;

        if (rc2.r>=0)
        {
            if (rc1.r<0)
            {
                //  購入
                int m = (j+1)*(j+1)*(j+1);
                j++;
                if (money<m)
                    return -1;
                money -= m;
            }
            else
            {
                //  移動
                if (!machine[rc1.r][rc1.c])
                    return -1;
                machine[rc1.r][rc1.c] = false;
            }

            if (machine[rc2.r][rc2.c])
                return -1;
            machine[rc2.r][rc2.c] = true;
        }

        for (int i=0; i<(int)SRC[t].size(); i++)
            vege[SRC[t][i].r][SRC[t][i].c] = SV[t][i];

        for (int r=0; r<N; r++)
            for (int c=0; c<N; c++)
                if (vege[r][c]>0 && machine[r][c])
                {
                    int v = vege[r][c];
                    vege[r][c] = 0;
                    int k = calc_k(machine, r, c);
                    money += (long long)v*k;
                }

        for (RC rc: ERC[t])
            vege[rc.r][rc.c] = 0;
    }

    return money;
}

int main()
{
    int _N, _M, _T;
    cin>>_N>>_M>>_T;
    for (int i=0; i<M; i++)
    {
        int R, C, S, E, V;
        cin>>R>>C>>S>>E>>V;
        SRC[S].push_back(RC(R, C));
        SV[S].push_back(V);
        ERC[E].push_back(RC(R, C));
    }

    chrono::system_clock::time_point start = chrono::system_clock::now();

    vector<Move> moves(T, Move(RC(-1, -1), RC(-1, -1)));
    long long score = 1;
    vector<Move> best_moves = moves;
    long long best_score = score;
    double time = 0;
    double temp = 1e10;
    int iter;
    for (iter=0; ; iter++)
    {
        if (iter%0x100==0)
        {
            chrono::system_clock::time_point now = chrono::system_clock::now();
            time = chrono::duration_cast<chrono::microseconds>(now-start).count()*1e-6/1.8;
            if (time>=1.)
                break;
            temp = (1-time)*1000;
        }

        int t = xor64()%T;
        Move old_move = moves[t];
        switch (xor64()%3)
        {
        case 0:
            moves[t].rc1.r = -1;
            moves[t].rc2 = RC(xor64()%N, xor64()%N);
            break;
        case 1:
            moves[t].rc1 = RC(xor64()%N, xor64()%N);
            moves[t].rc2 = RC(xor64()%N, xor64()%N);
            break;
        case 2:
            moves[t].rc2.r = -1;
        }

        long long score2 = calc_score(moves);
        if (score2<0)
        {
            moves[t] = old_move;
            continue;
        }

        if (score2>best_score)
        {
            best_moves = moves;
            best_score = score2;
        }

        double prob = exp((score2-score)/temp);
        //if (xor64()%0x10000/0x10000<prob)
        if (xor64()%0x10000<prob*0x10000)
            score = score2;
        else
            moves[t] = old_move;
    }

    for (Move move: moves)
    {
        if (move.rc2.r>=0)
            if (move.rc1.r<0)
                cout<<move.rc2.r<<" "<<move.rc2.c<<endl;
            else
                cout<<move.rc1.r<<" "<<move.rc1.c<<" "<<move.rc2.r<<" "<<move.rc2.c<<endl;
        else
            cout<<-1<<endl;
    }

    cerr<<"time: "<<time<<endl;
    cerr<<"iter: "<<iter<<endl;
    cerr<<"best_score: "<<best_score<<endl;
}
