#include <algorithm>
#include <iostream>
#include <queue>
#include <unordered_set>
#include <vector>
using namespace std;

int gcd(int a, int b) { return b ? gcd(b, a % b) : a; }

// Hash function based on wyhash by Wang Yi
// Source: https://github.com/wangyi-fudan/wyhash
// Author: Wang Yi <godspeed_china@yeah.net>
// License: Unlicense (Public Domain)

// Computes hash of water levels for O(1) lookup in visited set.
inline uint64_t wymum(uint64_t A, uint64_t B) {
	__uint128_t r = A;
	r *= B;
	return static_cast<uint64_t>((r >> 64) ^ r);
}

inline size_t computeHash(const vector<int> &level) {
	const uint64_t secret[4] = {0xa0761d6478bd642full, 0xe7037ed1a0b428dbull,
								0x8ebc6af09c88c6e3ull, 0x589965cc75374cc3ull};
	uint64_t seed = level.size();
	for (int val : level) {
		seed = wymum(seed ^ secret[0], static_cast<uint64_t>(val) ^ secret[1]);
	}
	return wymum(seed ^ secret[0], seed ^ secret[2]);
}

/*
 * Before running BFS, we can rule out impossible cases:
 * 1) Every target amount must be divisible by gcd of all capacities
 * 2) At least one glass must end up full or empty
 */
bool canPossiblyReach(const vector<int> &capacity, const vector<int> &target) {
	int n = static_cast<int>(capacity.size());

	int g = 0;
	for (int i = 0; i < n; i++)
		g = gcd(g, capacity[i]);

	for (int i = 0; i < n; i++) {
		if (target[i] % g != 0)
			return false;
	}

	for (int i = 0; i < n; i++) {
		if (target[i] == 0 || target[i] == capacity[i])
			return true;
	}
	return false;
}

// If target only requires full or empty glasses, just count the fills needed.
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
		return -1; // needs intermediate amount
	}
	return fillCount;
}

// Try a new state: check if it's the goal, otherwise queue it if unseen.
// Returns true if we found the goal.
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

// Handle pouring from glass i to all other glasses.
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

		int space = capacity[j] - cur[j];
		int poured = min(original, space);

		cur[i] = original - poured;
		cur[j] += poured;

		if (tryState(cur, steps, goalHash, visited, q, result))
			return true;

		cur[i] = original;
		cur[j] -= poured;
	}
	return false;
}

// Handle fill/empty operations on glass i.
inline bool tryFillEmpty(int i, vector<int> &cur, const vector<int> &capacity,
						 int steps, size_t goalHash,
						 unordered_set<size_t> &visited,
						 queue<pair<vector<int>, int>> &q, int &result) {
	int original = cur[i];

	// empty this glass
	if (original != 0) {
		cur[i] = 0;
		if (tryState(cur, steps, goalHash, visited, q, result))
			return true;
		cur[i] = original;
	}

	// fill this glass
	if (original != capacity[i]) {
		cur[i] = capacity[i];
		if (tryState(cur, steps, goalHash, visited, q, result))
			return true;
		cur[i] = original;
	}
	return false;
}

// BFS over the state space to find minimum moves.
int findMinimumMoves(const vector<int> &capacity, const vector<int> &target) {
	int n = static_cast<int>(capacity.size());
	size_t goalHash = computeHash(target);

	unordered_set<size_t> visited;
	queue<pair<vector<int>, int>> bfsQueue;

	vector<int> initial(n, 0);
	visited.insert(computeHash(initial));
	bfsQueue.push({move(initial), 0});

	while (!bfsQueue.empty()) {
		vector<int> cur = move(bfsQueue.front().first);
		int steps = bfsQueue.front().second + 1;
		bfsQueue.pop();

		int result = -1;
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

// Reads input and filters out zero-capacity glasses (they're useless).
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

// Check if we're already at target (all zeros).
inline bool isAlreadyAtTarget(const vector<int> &target) {
	for (int val : target) {
		if (val != 0)
			return false;
	}
	return true;
}

// Main solver logic. It first tries fast paths before falling back to BFS.
int solve(const vector<int> &capacity, const vector<int> &target) {
	if (capacity.empty() || isAlreadyAtTarget(target))
		return 0;

	if (!canPossiblyReach(capacity, target))
		return -1;

	int trivialAnswer = solveIfTrivial(capacity, target);
	if (trivialAnswer >= 0)
		return trivialAnswer;

	return findMinimumMoves(capacity, target);
}

int main() {
	ios::sync_with_stdio(false);
	cin.tie(nullptr);

	vector<int> capacity, target;
	readInput(capacity, target);
	cout << solve(capacity, target) << '\n';
}
