#include <algorithm>
#include <climits>
#include <cstdint>
#include <iostream>
#include <queue>
#include <unordered_set>
#include <vector>
using namespace std;

using ll = long long;

ll gcd(ll a, ll b) { return b ? gcd(b, a % b) : a; }

// Hash function based on wyhash by Wang Yi
// Source: https://github.com/wangyi-fudan/wyhash
// Author: Wang Yi <godspeed_china@yeah.net>
// License: Unlicense (Public Domain)

inline uint64_t wymum(uint64_t A, uint64_t B) {
	__uint128_t r = A;
	r *= B;
	return static_cast<uint64_t>((r >> 64) ^ r);
}

// Computes hash of water levels for O(1) lookup in visited set.
inline size_t computeHash(const vector<int> &level) {
	const uint64_t secret[3] = {0xa0761d6478bd642full, 0xe7037ed1a0b428dbull,
								0x8ebc6af09c88c6e3ull};
	uint64_t seed = level.size();
	for (int val : level) {
		seed = wymum(seed ^ secret[0], static_cast<uint64_t>(val) ^ secret[1]);
	}
	return wymum(seed ^ secret[0], seed ^ secret[2]);
}

// Check if the target is mathematically reachable:
// 1) all targets must be divisible by gcd of capacities
// 2) at least one glass must end up full or empty
bool canPossiblyReach(const vector<int> &capacity, const vector<int> &target) {
	int n = static_cast<int>(capacity.size());
	ll g = 0;
	for (int i = 0; i < n; i++)
		g = gcd(g, capacity[i]);
	for (int i = 0; i < n; i++)
		if (target[i] % g != 0)
			return false;
	for (int i = 0; i < n; i++)
		if (target[i] == 0 || target[i] == capacity[i])
			return true;
	return false;
}

// If all targets are just "empty" or "full", we can solve without BFS
int solveIfTrivial(const vector<int> &capacity, const vector<int> &target) {
	int n = static_cast<int>(capacity.size());
	int fillCount = 0;
	for (int i = 0; i < n; i++) {
		if (target[i] == 0)
			continue;
		if (target[i] == capacity[i]) {
			fillCount++;
			continue;
		}
		return -1; // needs intermediate amount, not trivial
	}
	return fillCount;
}

// Computes modular inverse using the extended Euclidean algorithm.
// Returns x such that (val * x) % mod == 1.
// Based on: https://en.wikipedia.org/wiki/Extended_Euclidean_algorithm
ll modInverse(ll val, ll mod) {
	ll x0 = 0, x1 = 1, a = val, b = mod;
	while (a > 1 && b > 0) {
		ll q = a / b;
		ll tmp = b;
		b = a - q * b;
		a = tmp;
		tmp = x0;
		x0 = x1 - q * x0;
		x1 = tmp;
	}
	return ((x1 % mod) + mod) % mod;
}

// Count ops when target ends up in the "to" jug (the one we pour into).
// k fills of "from", j empties of "to", with a pour after each. Formula: 2k +
// 2j
ll countOpsTargetInTo(ll from, ll to, ll target) {
	if (target == 0)
		return 0;
	if (target == to)
		return 1;

	ll d = gcd(from, to);
	if (target % d != 0)
		return LLONG_MAX;

	ll fd = from / d, td = to / d, tgt = target / d;
	ll inv = modInverse(fd, td);
	ll k = (tgt * inv) % td;
	if (k == 0)
		k = td;

	ll j = (k * from - target) / to;
	return 2 * k + 2 * j;
}

// Count ops when target ends up in the "from" jug (the one we fill).
// Formula: 2k + 2j - 1 (last empty leaves target in place, so there is no final
// pour)
ll countOpsTargetInFrom(ll from, ll to, ll target) {
	if (target == 0)
		return 0;
	if (target == from)
		return 1;

	ll d = gcd(from, to);
	if (target % d != 0)
		return LLONG_MAX;

	ll fd = from / d, td = to / d, tgt = target / d;
	ll inv = modInverse(fd, td);
	ll k = (tgt * inv) % td;
	if (k == 0)
		k = td;

	ll j = (k * from - target) / to;
	return 2 * k + 2 * j - 1;
}

// Count ops when target ends in "from" jug but "to" jug ends FULL (not empty).
// Formula: 2k + 2j (last pour fills "to" jug completely)
ll countOpsTargetInFromToFull(ll from, ll to, ll target) {
	if (target == 0)
		return 0;
	if (target == from)
		return 1;

	ll d = gcd(from, to);
	if (target % d != 0)
		return LLONG_MAX;

	ll fd = from / d, td = to / d, tgt = target / d;
	ll inv = modInverse(fd, td);
	ll k = (tgt * inv) % td;
	if (k == 0)
		k = td;

	// At the end, "from" holds target and "to" is full.
	// Total water = target + to, so we need k*from - j*to = target + to.
	ll j = (k * from - target - to) / to;
	if (j < 0)
		return LLONG_MAX; // not achievable with this k
	return 2 * k + 2 * j;
}

// For exactly 2 glasses, compute minimum operations using the water jug
// formulas. This avoids BFS which would be too slow for large capacities. We
// try all valid strategies and return the minimum operations.
ll solveForTwo(int a, int b, int ta, int tb) {
	if (ta == 0 && tb == 0)
		return 0;
	if (ta == a && tb == b)
		return 2;
	if (ta == a && tb == 0)
		return 1;
	if (ta == 0 && tb == b)
		return 1;

	ll best = LLONG_MAX;

	if (ta == 0) {
		// (0, tb): A empty, B has tb
		best = min(best, countOpsTargetInTo(a, b, tb));	  // fill A, pour to B
		best = min(best, countOpsTargetInFrom(b, a, tb)); // fill B, pour to A
	}

	if (tb == 0) {
		// (ta, 0): A has ta, B empty
		best = min(best, countOpsTargetInTo(b, a, ta));	  // fill B, pour to A
		best = min(best, countOpsTargetInFrom(a, b, ta)); // fill A, pour to B
	}

	if (ta == a) {
		// (a, tb): A full, B has tb
		best = min(best, countOpsTargetInTo(a, b, tb) + 1);
		best = min(best, countOpsTargetInFrom(b, a, tb) + 1);
		// A gets full naturally when pouring B→A: need (a+tb) water total
		if ((a + tb) % b == 0) {
			ll k = (a + tb) / b;
			best = min(best, 2 * k);
		}
		// B has tb, A ends full after last pour (fill B, pour to A strategy)
		best = min(best, countOpsTargetInFromToFull(b, a, tb));
	}

	if (tb == b) {
		// (ta, b): A has ta, B full
		best = min(best, countOpsTargetInTo(b, a, ta) + 1);
		best = min(best, countOpsTargetInFrom(a, b, ta) + 1);
		// B gets full naturally when pouring A→B: need (b+ta) water total
		if ((b + ta) % a == 0) {
			ll k = (b + ta) / a;
			best = min(best, 2 * k);
		}
		// A has ta, B ends full after last pour (fill A, pour to B strategy)
		best = min(best, countOpsTargetInFromToFull(a, b, ta));
	}

	return best;
}

// Try a new state: if it's the goal return true, otherwise add to queue
inline bool tryState(vector<int> &cur, int steps, size_t goalHash,
					 unordered_set<size_t> &visited,
					 queue<pair<vector<int>, int>> &q, int &result) {
	size_t h = computeHash(cur);
	if (h == goalHash) {
		result = steps;
		return true;
	}
	if (visited.insert(h).second)
		q.push({cur, steps});
	return false;
}

// BFS helper. We try pouring water from glass i into every other glass.
// For each valid pour (target not full, source not empty), temporarily modify
// the state, check if it's the goal, add to queue if new, then undo the change.
inline bool tryPourFrom(int i, vector<int> &cur, const vector<int> &capacity,
						int steps, size_t goalHash,
						unordered_set<size_t> &visited,
						queue<pair<vector<int>, int>> &q, int &result) {
	int n = static_cast<int>(capacity.size());
	int original = cur[i];
	if (original == 0)
		return false;

	for (int j = 0; j < n; j++) {
		if (j == i || cur[j] == capacity[j])
			continue;

		int poured = min(original, capacity[j] - cur[j]);
		cur[i] = original - poured;
		cur[j] += poured;

		if (tryState(cur, steps, goalHash, visited, q, result))
			return true;

		cur[i] = original;
		cur[j] -= poured;
	}
	return false;
}

// BFS helper. We try emptying glass i (set to 0) and filling it (set to
// capacity). Each valid action is a new state. We check if it's the goal, add
// to queue if not visited, then restore the original value before trying the
// next action.
inline bool tryFillEmpty(int i, vector<int> &cur, const vector<int> &capacity,
						 int steps, size_t goalHash,
						 unordered_set<size_t> &visited,
						 queue<pair<vector<int>, int>> &q, int &result) {
	int original = cur[i];

	if (original != 0) {
		cur[i] = 0;
		if (tryState(cur, steps, goalHash, visited, q, result))
			return true;
		cur[i] = original;
	}
	if (original != capacity[i]) {
		cur[i] = capacity[i];
		if (tryState(cur, steps, goalHash, visited, q, result))
			return true;
		cur[i] = original;
	}
	return false;
}

// Standard BFS for n >= 3 glasses
int bfsSolve(const vector<int> &capacity, const vector<int> &target) {
	int n = static_cast<int>(capacity.size());
	size_t goalHash = computeHash(target);
	unordered_set<size_t> visited;
	queue<pair<vector<int>, int>> bfsQueue;

	vector<int> initial(n, 0);
	visited.insert(computeHash(initial));
	bfsQueue.push({std::move(initial), 0});

	while (!bfsQueue.empty()) {
		vector<int> cur = std::move(bfsQueue.front().first);
		int steps = bfsQueue.front().second + 1;
		bfsQueue.pop();

		int result;
		for (int i = 0; i < n; i++) {
			if (tryFillEmpty(i, cur, capacity, steps, goalHash, visited,
							 bfsQueue, result))
				return result;
			if (tryPourFrom(i, cur, capacity, steps, goalHash, visited,
							bfsQueue, result))
				return result;
		}
	}
	return -1;
}

// Read input, skipping zero-capacity glasses
inline void readInput(vector<int> &capacity, vector<int> &target) {
	int n;
	cin >> n;
	capacity.reserve(n);
	target.reserve(n);
	for (int i = 0; i < n; i++) {
		int cap, goal;
		cin >> cap >> goal;
		if (cap > 0) {
			capacity.push_back(cap);
			target.push_back(goal);
		}
	}
}

// Check if target is all zeros (already satisfied)
inline bool isAlreadyAtTarget(const vector<int> &target) {
	for (int val : target)
		if (val != 0)
			return false;
	return true;
}

// Main solver. It returns minimum operations to reach target state, or -1 if
// impossible. Uses mathematical pruning, trivial case detection, closed-form
// for n=2, and BFS for n>=3.
int solve(const vector<int> &capacity, const vector<int> &target) {
	if (capacity.empty() || isAlreadyAtTarget(target))
		return 0;
	if (!canPossiblyReach(capacity, target))
		return -1;

	int trivialAnswer = solveIfTrivial(capacity, target);
	if (trivialAnswer >= 0)
		return trivialAnswer;

	if (capacity.size() == 2) {
		ll ans = solveForTwo(capacity[0], capacity[1], target[0], target[1]);
		return static_cast<int>(ans);
	}
	return bfsSolve(capacity, target);
}

int main() {
	ios::sync_with_stdio(false);
	cin.tie(nullptr);

	vector<int> capacity, target;
	readInput(capacity, target);
	cout << solve(capacity, target) << '\n';
}
