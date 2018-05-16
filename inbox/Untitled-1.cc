#include <iostream>
using namespace std;



int main{
    
    int m, n;
    cin >> m >>n;
    vector<pair<int, int>> task;
    vector<pair<int, int>> mh;
    int dp[n][m]
    {
    int i = m;
    while(i-- > 0){
        std::pair<int, int> p;
        cin >> p.first >> p.second;
        task.push_pach(p);
    }

    i = n;
    while(i-->0) {
        std::pair<int, int> p;
        cin >> p.first >> p.second;
        mh.push_pach(p);
    }
    }
    int mmax = 0;
    for(int i= 0;i < n; i++) {
        for(int j = 0; j < m; j++) {
            if(i ==0 && j == 0) {
                if(task[i].first <= mh[i].first && task[i].second <= mh[i].second) {
                    dp[i][j] = 100 * task[i].first +3*task.second;
                } else {
                    dp[i][j] = 0;
                }
            } else if(i == 0) {
                if(task[i].first <= mh[i].first && task[i].second <= mh[i].second) {
                    dp[i][j] = std::min(100 * task[i].first +3*task.second, dp[i][j-1]);
                } else {
                    dp[i][j] = dp[i][j-1];
                }
            } else if(j == 0) {
                if(task[i].first <= mh[i].first && task[i].second <= mh[i].second) {
                    dp[i][j] = std::min(100 * task[i].first +3*task.second, dp[i-1][j]);
                } else {
                    dp[i][j] = dp[i-1][j];
                }
            } else {
                if(task[i].first <= mh[i].first && task[i].second <= mh[i].second) {
                    dp[i][j] = 100 * task[i].first +3*task.second + dp[i-1][j-1];
                } else {
                    dp[i][j] = std::min(dp[i-1][j], dp[i][j-1]);
                }
            }
            if(dp[i][j]>mmax)
                mmax = dp[i][j];
        }
    }
    cout << mmax;
}