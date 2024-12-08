https://codeforces.com/problemset/problem/547/B
#include <bits/stdc++.h>

using namespace std;

using i64 = long long;

int main() {
    ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    int n;
    cin >> n;

    vector<int> a(n + 1);
    for (int i = 1; i <= n; i ++) {
        cin >> a[i];
    }

    vector<int> stk;
    vector<int> l(n + 1), r(n + 1);
    for (int i = 1; i <= n; i ++) {
        while (!stk.empty() && a[i] <= a[stk.back()]) {
            stk.pop_back();
        }
        l[i] = stk.empty() ? 0 : stk.back();
        stk.push_back(i);
    }

    stk.clear();
    for (int i = n; i >= 1; i --) {
        while (!stk.empty() && a[i] <= a[stk.back()]) {
            stk.pop_back();
        }
        r[i] = stk.empty() ? n + 1 : stk.back();
        stk.push_back(i);
    }

    vector<int> dp(n + 1);
    for (int i = 1; i <= n; i ++) {
        int len = r[i] - l[i] - 1;
        dp[len] = max(dp[len], a[i]);
    }

    for (int i = n; i >= 1; i --) {
        dp[i - 1] = max(dp[i - 1], dp[i]);
    }

    for (int i = 1; i <= n; i ++) {
        cout << dp[i] << " ";
    }

    return 0;
}