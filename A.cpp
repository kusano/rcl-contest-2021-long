//  g++ -O2 -o A A.cpp && python3 test.py

#include <iostream>
#include <vector>
#include <cassert>
#include <chrono>
#include <cmath>
#include <queue>
#include <cstring>
#include <algorithm>
#include <set>
using namespace std;

const int N = 16;
const int M = 5000;
const int T = 1000;

//  これ以降は、収穫機を買わない
const int T_THRES = 850;

struct Move
{
    //  購入はfrom=-1
    //  パスはto=-1
    int from, to;
    Move(): from(-1), to(-1) {}
    Move(int from, int to): from(from), to(to) {}
};

struct State
{
    short field[N*N] = {};
    bool machine[N*N] = {};
    unsigned long long hash = 0;
    long long score = 0;
    long long money = 0;
    int machine_number = 0;
    State *prev = nullptr;
    Move move;
};

bool operator<(const State &s1, const State &s2)
{
    return s1.score>s2.score;
}
bool cmp_state(const State *s1, const State *s2)
{
    return *s1<*s2;
}

struct PosV
{
    int pos;
    int v;
    PosV(int pos, int v): pos(pos), v(v) {}
};

unsigned long long Hash[N*N];
unsigned char Neighbor[N*N][4]; // Neighbor[i][x]==i は無効値
int MachinePrice[N*N];          //  i番目の収穫機を買うときの値段
int MachineTotal[N*N+1];        //  i個の収穫機を買うのに使った金額
int MoneySum[T+N*N+1][N*N];     //  得られる金額の累積和

vector<PosV> SPosV[T];
vector<int> EPos[T];

int xor64(void) {
    static uint64_t x = 88172645463325252ULL;
    x ^= x<<13;
    x ^= x>> 7;
    x ^= x<<17;
    return int(x&0x7fffffff);
}

long long calc_hash(bool M[N*N])
{
    long long h = 0;
    for (int p=0; p<N*N; p++)
        if (M[p])
            h ^= Hash[p];
    return h;
}

long long calc_score(int turn, const State &s, const vector<int> &machine_pos)
{
    long long score;
    //  最後は金を見る
    if (turn>=T_THRES)
        score = s.money;
    else
        score = s.money + MachineTotal[s.machine_number];
    score <<= 8;

    for (int p: machine_pos)
        if (s.machine[p])
            score += MoneySum[turn+s.machine_number+2][p] - MoneySum[turn+2][p];
    score <<= 8;

    score += (long long)(s.hash&0xff);
    return score;
}

void init()
{
    for (int p=0; p<N*N; p++)
        Hash[p] = (unsigned long long)xor64()<<32 | xor64();

    int dr[] = {1, -1, 0, 0};
    int dc[] = {0, 0, 1, -1};

    for (int p=0; p<N*N; p++)
    {
        int r = p/N;
        int c = p%N;
        for (int d=0; d<4; d++)
        {
            int tr = r+dr[d];
            int tc = c+dc[d];
            if (0<=tr && tr<N &&
                0<=tc && tc<N)
                Neighbor[p][d] = (unsigned char)(tr*N+tc);
            else
                Neighbor[p][d] = p;
        }
    }

    for (int i=0; i<N*N; i++)
        MachinePrice[i] = (i+1)*(i+1)*(i+1);
    MachineTotal[0] = 0;
    for (int i=1; i<=N*N; i++)
        MachineTotal[i] = MachineTotal[i-1]+MachinePrice[i-1];

    for (int t=0; t<T; t++)
    {
        for (int i=0; i<N*N; i++)
            MoneySum[t+1][i] = MoneySum[t][i];
        for (PosV posv: SPosV[t])
        {
            MoneySum[t+1][posv.pos] += posv.v*16;
            for (int t: Neighbor[posv.pos])
                if (t!=posv.pos)
                    MoneySum[t+1][t] += posv.v;
        }
    }
    for (int t=T; t<T+N*N; t++)
        for (int i=0; i<N*N; i++)
            MoneySum[t+1][i] = MoneySum[t][i];
}

int main()
{
    int _N, _M, _T;
    cin>>_N>>_M>>_T;
    for (int i=0; i<M; i++)
    {
        int R, C, S, E, V;
        cin>>R>>C>>S>>E>>V;
        SPosV[S].push_back(PosV(R*N+C, V));
        EPos[E].push_back(R*N+C);
    }

    chrono::system_clock::time_point start = chrono::system_clock::now();

    init();

    const int BW = 256;
    vector<State> S[T];

    //  収穫機があるかもしれない位置
    //  calc_scoreに補助情報として渡す
    vector<int> machine_pos;

    //  t=0    
    for (int i=0; i<N*N; i++)
        machine_pos.push_back(i);

    for (int p=0; p<N*N; p++)
    {
        State s;
        s.score = 1;
        s.machine[p] = true;
        s.move = Move(-1, p);
        s.machine_number = 1;
        for (auto posv: SPosV[0])
        {
            if (s.machine[posv.pos])
                s.money += posv.v;
            else
                s.field[posv.pos] = posv.v;
        }
        for (int p: EPos[0])
            s.field[p] = 0;
        s.hash = calc_hash(s.machine);
        s.score = calc_score(0, s, machine_pos);

        S[0].push_back(s);
    }

    sort(S[0].begin(), S[0].end());
    if ((int)S[0].size()>BW)
        S[0].resize(BW);

    vector<State> Snew;
    vector<State *> Stemp;
    multiset<long long> score;
    vector<int> from;

    for (int turn=1; turn<T; turn++)
    {
        Snew.clear();
        score.clear();

        for (State &s1: S[turn-1])
        {
            machine_pos.clear();
            for (int p=0; p<N*N; p++)
                if (s1.machine[p])
                    machine_pos.push_back(p);

            from.clear();
            //  最後は収穫機を買わない
            //  それ以前は買えるときには必ず買う
            if (turn<T_THRES &&
                MachinePrice[s1.machine_number]<=s1.money)
                from.push_back(-1);
            else
            {
                //  この時点では必ず収穫機があるので、s1.machine[p]のチェックは不要
                for (int p: machine_pos)
                {
                    int nn = 0;
                    int np[4];
                    for (int t: Neighbor[p])
                        if (t!=p &&
                            s1.machine[t])
                            np[nn++] = t;

                    //  隣接収穫機数が1個のもののみ移動可
                    //  ただし、2個でも間に収穫機があれば可
                    if (s1.machine_number==1 ||
                        nn==1 ||
                        nn==2 && np[0]+np[1]-p!=p && s1.machine[np[0]+np[1]-p])
                        from.push_back(p);
                }
            }

            for (int t=0; t<N*N; t++)
            {
                if (s1.machine[t])
                    continue;

                //  隣接収穫機数
                int nn = 0;
                int np[4] = {-1};
                for (int tt: Neighbor[t])
                    if (tt!=t &&
                        s1.machine[tt])
                        np[nn++] = tt;

                if (s1.machine_number>1)
                {
                    //  木の葉の位置のみに置く
                    //  ただし、4個を正方形に並べるのは可
                    if (nn!=1 && nn!=2)
                        continue;
                    if (nn==2 && !s1.machine[np[0]+np[1]-t])
                        continue;
                }

                //  移動元の選択
                int fn = (int)from.size();
                int fr = xor64()%fn;
                int f = -2;
                //  隣接収穫機数が2個以上、もしくは隣接した収穫機ではない
                if (nn>=2 || from[fr]!=np[0])
                    f = from[fr];
                else
                    for (int ff: from)
                        if (ff!=np[0])
                        {
                            f = ff;
                            break;
                        }
                if (f==-2)
                    continue;

                State s2 = s1;
                s2.prev = &s1;
                s2.move = Move(f, t);

                if (f>=0)
                {
                    s2.machine[f] = false;
                    s2.hash ^= Hash[f];
                }
                s2.machine[t] = true;
                s2.hash ^= Hash[t];
                machine_pos.push_back(t);

                if (f<0)
                {
                    s2.money -= MachinePrice[s2.machine_number];
                    s2.machine_number++;
                }

                if (s2.field[t]>0)
                {
                    s2.money += s2.field[t]*s2.machine_number;
                    s2.field[t] = 0;
                }
                for (auto posv: SPosV[turn])
                {
                    if (s2.machine[posv.pos])
                        s2.money += posv.v*s2.machine_number;
                    else
                        s2.field[posv.pos] = posv.v;
                }
                for (int p: EPos[turn])
                    s2.field[p] = 0;

                s2.score = calc_score(turn, s2, machine_pos);

                machine_pos.pop_back();

                if (score.size()<BW ||
                    s2.score>*score.begin())
                {
                    Snew.push_back(s2);
                    score.insert(s2.score);
                    if (score.size()>BW)
                        score.erase(score.begin());
                }
            }
        }

        Stemp.clear();
        for (State &s: Snew)
            Stemp.push_back(&s);
        sort(Stemp.begin(), Stemp.end(), cmp_state);
        if ((int)Stemp.size()>BW)
            Stemp.resize(BW);
        for (State *s: Stemp)
            S[turn].push_back(*s);
    }

    cerr<<"money: "<<S[T-1][0].money<<endl;
    int mn = 0;
    for (int m: S[T-1][0].machine)
        if (m)
            mn++;
    cerr<<"machine: "<<mn<<endl;

    vector<Move> moves;
    for (State *s = &S[T-1][0]; s!=nullptr; s=s->prev)
        moves.push_back(s->move);
    reverse(moves.begin(), moves.end());

    for (Move move: moves)
        if (move.to>=0)
            if (move.from<0)
                cout<<move.to/N<<" "<<move.to%N<<endl;
            else
                cout<<move.from/N<<" "<<move.from%N<<" "<<move.to/N<<" "<<move.to%N<<endl;
        else
            cout<<-1<<endl;

    cerr<<"time: "<<chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now()-start).count()*1e-6<<endl;
}
