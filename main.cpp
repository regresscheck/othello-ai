// First version of strategy. Plays with itself since no communication format for programs given yet.

#include <iostream>
#include <vector>
#include <climits>

using namespace std;

const int FIELD_SIZE = 8;
const int EVALUATION_DISK_MULTIPLIER = 1;
const int EVALUATION_MOVE_MULTIPLIER = 100;
const int EVALUATION_CORNER_MULTIPLIER = 1000;
const int SEARCH_DEPTH = 5;

int sign(int x) {
    if (x < 0)
        return -1;
    if (x > 0)
        return 1;
    return 0;
}

struct Position {
    int x, y;
    // sets impossible coordinates by default
    explicit Position(int x = -1, int y = -1): x(x), y(y) {};
    bool isCorrect() const {
        return (x >= 0 && x < FIELD_SIZE && y >= 0 && y < FIELD_SIZE);
    }
    void update(int dx, int dy) {
        x += dx;
        y += dy;
    }
    bool operator!=(const Position & other) const {
        return x != other.x || y != other.y;
    }
};

class OthelloState {
public:
    enum FieldState {EMPTY, WHITE, BLACK};
private:
    FieldState field[FIELD_SIZE][FIELD_SIZE];
    FieldState currentPlayer;
public:
    OthelloState() {
        for (int i = 0; i < FIELD_SIZE; i++)
            for (int j = 0; j < FIELD_SIZE; j++)
                field[i][j] = FieldState::EMPTY;
        // constant indexes by rules
        field[3][3] = field[4][4] = FieldState::WHITE;
        field[3][4] = field[4][3] = FieldState::BLACK;
        currentPlayer = FieldState::BLACK;
    }
    FieldState & getField(const Position & position) {
        return field[position.x][position.y];
    }
    FieldState getField(const Position & position) const {
        return field[position.x][position.y];
    }

    // Finds first correct disk in given direction or return incorrect position
    Position findSameColorDisk(Position position, int dx, int dy) const {
        position.update(dx, dy);
        bool isCorrect_position = false;
        for (position; position.isCorrect(); position.update(dx, dy)) {
            if (getField(position) == currentPlayer) {
                if (isCorrect_position)
                    return position;
                else
                    return Position();
            } else
                if (getField(position) == FieldState::EMPTY)
                    return Position();
                else
                    isCorrect_position = true;
        }

        return Position();
    }
    void reverseField(const Position & position) {
        if (getField(position) == FieldState::BLACK)
            getField(position) = FieldState::WHITE;
        else
            if (getField(position) == FieldState::WHITE)
                getField(position) = FieldState::BLACK;
    }
    void reverseLine(Position start_position, const Position & end_position) {
        int dx = sign(end_position.x - start_position.x);
        int dy = sign(end_position.y - start_position.y);
        start_position.update(dx, dy);
        for (start_position; start_position != end_position; start_position.update(dx, dy)) {
            reverseField(start_position);
        }
    }

    void changePlayer() {
        if (currentPlayer == FieldState::WHITE)
            currentPlayer = FieldState::BLACK;
        else
            currentPlayer = FieldState::WHITE;
    }
    void putDisk(const Position & position) {
        for (int dx = -1; dx <= 1; dx++)
            for (int dy = -1; dy <= 1; dy++)
                if (dx != 0 || dy != 0)
                {
                    Position next_position = findSameColorDisk(position, dx, dy);
                    if (next_position.isCorrect()) {
                        reverseLine(position, next_position);
                    }
                }
        getField(position) = currentPlayer;
        changePlayer();
    }
    bool isPossibleMove(const Position & position) const {
        if (getField(position) != FieldState::EMPTY)
            return false;
        for (int dx = -1; dx <= 1; dx++)
            for (int dy = -1; dy <= 1; dy++)
                if (dx != 0 || dy != 0)
                {
                    Position next_position = findSameColorDisk(position, dx, dy);
                    if (next_position.isCorrect()) {
                        return true;
                    }
                }
        return false;
    }
    bool isPossibleMove(int x, int y) const {
        Position position(x, y);
        return isPossibleMove(position);
    }
    vector<Position> getPossibleMoves() const {
        vector<Position> possible_moves;
        for (int x = 0; x < FIELD_SIZE; x++)
            for (int y = 0; y < FIELD_SIZE; y++)
                if (isPossibleMove(x, y))
                    possible_moves.push_back(Position(x, y));
        return possible_moves;
    }
    int getBalance() const {
        int result = 0;
        for (int x = 0; x < FIELD_SIZE; x++)
            for (int y = 0; y < FIELD_SIZE; y++) {
                if (field[x][y] == currentPlayer)
                    result++;
                else
                    if (field[x][y] != FieldState::EMPTY)
                        result--;
            }
        return result;
    }
    long long evaluate() const {
        long long result = 0;
        for (int x = 0; x < FIELD_SIZE; x++)
            for (int y = 0; y < FIELD_SIZE; y++) {
                if (field[x][y] == currentPlayer)
                    result += EVALUATION_DISK_MULTIPLIER;
                else
                    if (field[x][y] != FieldState::EMPTY)
                        result -= EVALUATION_DISK_MULTIPLIER;
            }
        result += getPossibleMoves().size() * EVALUATION_MOVE_MULTIPLIER;

        // iterates all corners
        for (int x = 0; x < FIELD_SIZE; x += FIELD_SIZE - 1)
            for (int y = 0; y < FIELD_SIZE; y += FIELD_SIZE - 1) {
                if (field[x][y] == currentPlayer)
                    result += EVALUATION_CORNER_MULTIPLIER;
                else
                    if (field[x][y] != FieldState::EMPTY)
                        result -= EVALUATION_CORNER_MULTIPLIER;
            }
        return result;
    }
};

struct AlphabetaResult {
    Position position;
    long long value;
    explicit AlphabetaResult(long long value, Position position): value(value), position(position) {}
    explicit AlphabetaResult() {}
};

bool operator<(const AlphabetaResult & first, const AlphabetaResult & second) {
    return first.value < second.value;
}

// finds best move by A/B pruning
AlphabetaResult alphabeta(const OthelloState state, int depth, long long alpha, long long beta, bool maximizing) {
    if (depth == 0)
        return AlphabetaResult(state.evaluate(), Position());
    vector<Position> possible_moves = state.getPossibleMoves();
    if (possible_moves.size() == 0)
        return AlphabetaResult(state.evaluate(), Position());
    AlphabetaResult result, current;
    OthelloState new_state;
    if (maximizing) {
        result.value = LLONG_MIN;
        for (int i = 0; i < possible_moves.size(); i++) {
            new_state = state;
            new_state.putDisk(possible_moves[i]);
            current = alphabeta(new_state, depth - 1, alpha, beta, false);
            if (result.value < current.value) {
                result.value = current.value;
                result.position = possible_moves[i];
            }
            alpha = max(alpha, result.value);
            if (beta < alpha)
                break;
        }
        return result;
    } else {
        result.value = LLONG_MAX;
        for (int i = 0; i < possible_moves.size(); i++) {
            new_state = state;
            new_state.putDisk(possible_moves[i]);
            current = alphabeta(new_state, depth - 1, alpha, beta, true);
            if (current.value < result.value) {
                result.value = current.value;
                result.position = possible_moves[i];
            }
            beta = min(beta, result.value);
            if (beta < alpha)
                break;
        }
        return result;
    }
}

void printState(const OthelloState & state) {
    for (int i = 0; i < FIELD_SIZE; i++) {
        for (int j = 0; j < FIELD_SIZE; j++) {
            if (state.getField(Position(i, j)) == OthelloState::FieldState::EMPTY)
                cout << '#';
            if (state.getField(Position(i, j)) == OthelloState::FieldState::BLACK)
                cout << 'B';
            if (state.getField(Position(i, j)) == OthelloState::FieldState::WHITE)
                cout << 'W';

        }
        cout << endl;
    }
    cout << endl;
}

int main()
{

    OthelloState state;
    int moves_count = 0;
    int bad_moves_count = 0;
    int x, y;
    while (true) {
        cout << "MOVE #" << moves_count << ". BALANCE: " << state.getBalance() << endl;
        printState(state);
        moves_count++;
        AlphabetaResult result;
        result = alphabeta(state, SEARCH_DEPTH, LLONG_MIN, LLONG_MAX, true);
        if (result.position.isCorrect()) {
            state.putDisk(result.position);
            bad_moves_count = 0;
        } else {
            state.changePlayer();
            bad_moves_count++;
        }

        // both players can't move
        if (bad_moves_count == 2) {
            break;
        }
    }
    return 0;
}
