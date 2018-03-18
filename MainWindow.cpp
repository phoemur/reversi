#include <QString>
#include <QMessageBox>
#include <algorithm>

#include "MainWindow.h"

MainWindow::MainWindow(QMainWindow* parent)
    : QMainWindow(parent), 
      ui{std::make_unique<Ui::MainWindow>()},
      signalMapper{new QSignalMapper(this)},
      board(SIZE, std::vector<char>(SIZE, ' ')),
      btn_storage(SIZE, std::vector<QPushButton*>(SIZE, nullptr)),
      engine(this->seeder())
{
    ui->setupUi(this);
    
    // Put pieces on central cells
    // Black is Minimizer player, W is Maximizer
    board[3][3] = 'W';
    board[4][4] = 'W';
    board[3][4] = 'B';
    board[4][3] = 'B';
    
    // Make icons
    black.addPixmap(QPixmap("icons/black.png"), QIcon::Disabled);
    white.addPixmap(QPixmap("icons/white.png"), QIcon::Disabled);
    
    
    // Create buttons
    for (int i=0; i<SIZE; ++i) {
        for (int j=0; j<SIZE; ++j) {
            QPushButton* btn = new QPushButton(ui->centralwidget);
            btn->setMinimumWidth(64);
            btn->setMinimumHeight(64);
            btn->setAttribute(Qt::WA_LayoutUsesWidgetRect);
            btn->setIconSize(QSize(48, 48));
            
            if (board[i][j] == 'W') {
                btn->setIcon(white);
                btn->setEnabled(false);
            }
            else if (board[i][j] == 'B') {
                btn->setIcon(black);
                btn->setEnabled(false);
            }
            
            ui->gridLayout->addWidget(btn, i, j);
            QString coordinates = QString::number(i)+","+QString::number(j); //Coordinate of the button
            signalMapper->setMapping(btn, coordinates);
            
            connect(btn, SIGNAL(clicked()), signalMapper, SLOT(map()));
            
            btn_storage[i][j] = btn; // store pointer on array for easy retrieval
        }
    }
    
    connect(signalMapper, SIGNAL(mapped(QString)), this, SLOT(buttonClicked(QString)));
    
    // Statusbar
    update_scores();
    lab.setStyleSheet("font-weight: bold; color: black");
    ui->statusbar->addPermanentWidget(&lab);
    update_status_bar();
}

void MainWindow::buttonClicked(QString coordinates)
{    
    QStringList results = coordinates.split(",");
    int row = results.at(0).toInt();
    int col = results.at(1).toInt();
    
    if (!is_valid_move(board, row, col, false)) {
        QMessageBox::warning(this, "Invalid", "Invalid move");
        return;
    }
    
    // Your turn 
    make_move(board, row, col, false); // make move
    update_icons();
    update_scores();
    update_status_bar();
    
    if (check_end()) {
        block_all_cells();
        
        if (my_score > computer_score) {
            QMessageBox::information(this, "Congratulations", "You Win!!!");
            return;
        }
        else if (my_score < computer_score) {
            QMessageBox::warning(this, "Ouch!", "You Loose!!");
            return;
        }
        else {
            QMessageBox::warning(this, "Ouch!", "DRAW!!!");
            return;
        }
    }
    
    if (!has_moves_available(board, true)) { // if computer has no moves available play again
        QMessageBox::information(this, "Play again", "Computer has no moves available.\n Play again.");
        return;
    }
    
    // Computer turn
    do {
        auto coord = computer_move(true);
        make_move(board, coord.first, coord.second, true);
        update_icons();
        update_scores();
        update_status_bar();
        
        if (check_end()) {
            block_all_cells();
            
            if (my_score > computer_score) {
                QMessageBox::information(this, "Congratulations", "You Win!!!");
                return;
            }
            else if (my_score < computer_score) {
                QMessageBox::warning(this, "Ouch!", "You Loose!!");
                return;
            }
            else {
                QMessageBox::warning(this, "Ouch!", "DRAW!!!");
                return;
            }
        }
        
    } while (!has_moves_available(board, false));
}

inline bool MainWindow::is_safe_coord(const int row, 
                                      const int col) const noexcept
{
    return row >=0 && row < SIZE && col >= 0 && col < SIZE;
}

bool MainWindow::valid_move_on_row(const std::vector<std::vector<char>>& table,
                                   const int row, 
                                   const int col, 
                                   const bool isMax) const noexcept
{
    char player = 'W', opponent = 'B';
    if (!isMax) {std::swap(player, opponent);}
    
    // check right side
    for (int i = col+1; i<SIZE; ++i) {
        if (table[row][i] == player) {
            if (i - col > 1) {return true;}
            else {break;}
        }
        else if (table[row][i] == ' ') {break;}
    }
    
    // check left side
    for (int i = col-1; i>=0; --i) {
        if (table[row][i] == player) {
            if (col - i > 1) {return true;}
            else {break;}
        }
        else if (table[row][i] == ' ') {break;}
    }
    
    return false;
}

bool MainWindow::valid_move_on_col(const std::vector<std::vector<char>>& table,
                                   const int row, 
                                   const int col, 
                                   const bool isMax) const noexcept
{
    char player = 'W', opponent = 'B';
    if (!isMax) {std::swap(player, opponent);}
    
    // check below
    for (int i = row+1; i<SIZE; ++i) {
        if (table[i][col] == player) {
            if (i - row > 1) {return true;}
            else {break;}
        }
        else if (table[i][col] == ' ') {break;}
    }
    
    // check above    
    for (int i = row-1; i>=0; --i) {
        if (table[i][col] == player) {
            if (row - i > 1) {return true;}
            else {break;}
        }
        else if (table[i][col] == ' ') {break;}
    }
    
    return false;
}

bool MainWindow::valid_move_on_diag(const std::vector<std::vector<char>>& table,
                                    const int row, 
                                    const int col, 
                                    const bool isMax) const noexcept
{
    char player = 'W', opponent = 'B';
    if (!isMax) {std::swap(player, opponent);}
    
    // check right-side diagonals
    bool upper_valid = true;
    bool lower_valid = true;
    
    for (int i = col+1, j=1; i<SIZE; ++i, ++j) {
        if (is_safe_coord(row+j, i)) {
            if (table[row+j][i] == player && lower_valid) {
                if (i - col > 1) {return true;}
                else {lower_valid = false;}
            }
            else if (table[row+j][i] == ' ') {lower_valid = false;}
        }
        
        if (is_safe_coord(row-j, i)) {
            if (table[row-j][i] == player && upper_valid) {
                if (i - col > 1) {return true;}
                else {upper_valid = false;}
            }
            else if (table[row-j][i] == ' ') {upper_valid = false;}
        }
    }
    
    // check left-side diagonals
    upper_valid = true;
    lower_valid = true;
    
    for (int i = col-1, j=1; i>=0; --i, ++j) {
        if (is_safe_coord(row+j, i)) {
            if (table[row+j][i] == player && lower_valid) {
                if (col - i > 1) {return true;}
                else {lower_valid = false;}
            }
            else if (table[row+j][i] == ' ') {lower_valid = false;}
        }
        
        if (is_safe_coord(row-j, i)) {
            if (table[row-j][i] == player && upper_valid) {
                if (col - i > 1) {return true;}
                else {upper_valid = false;}
            }
            else if (table[row-j][i] == ' ') {upper_valid = false;}
        }
    }
    
    
    return false;
}

bool MainWindow::is_valid_move(const std::vector<std::vector<char>>& table,
                               const int row, 
                               const int col, 
                               const bool isMax) const noexcept
{
    return valid_move_on_row(table, row, col, isMax) ||
           valid_move_on_col(table, row, col, isMax) ||
           valid_move_on_diag(table, row, col, isMax);
}

inline void MainWindow::update_icons()
{
    for (int i=0; i<SIZE; ++i) {
        for (int j=0; j<SIZE; ++j) {
            if (board[i][j] == 'W') {
                QPushButton* btn_pushed = btn_storage[i][j];
                btn_pushed->setIcon(white);
                btn_pushed->setEnabled(false);
            }
            else if (board[i][j] == 'B') {
                QPushButton* btn_pushed = btn_storage[i][j];
                btn_pushed->setIcon(black);
                btn_pushed->setEnabled(false);
            }
        }
    }
}

void MainWindow::flip_pieces_on_row(std::vector<std::vector<char>>& table,
                                    const int row, 
                                    const int col, 
                                    const bool isMax)
{
    char player = 'W', opponent = 'B';
    if (!isMax) {std::swap(player, opponent);}
    
    // right side
    bool found = false;
    int i = col+1;
    for (; i<SIZE; ++i) {
        if (table[row][i] == player) {
            if (i - col > 1) {
                found = true;
                break;
            }
            else {break;}
        }
        else if (table[row][i] == ' ') {break;}
    }
    
    if (found) {
        for (int j = col+1; j<i; ++j) {
            table[row][j] = player;
        }
    }
        
    // left side
    i = col - 1;
    found = false;
    for (; i>=0; --i) {
        if (table[row][i] == player) {
            if (col - i > 1) {
                found = true;
                break;
            }
            else {break;}
        }
        else if (table[row][i] == ' ') {break;}
    }
    if (found) {
        for (int j = col-1; j > i; --j) {
            table[row][j] = player;
        }            
    }
}

void MainWindow::flip_pieces_on_col(std::vector<std::vector<char>>& table,
                                    const int row, 
                                    const int col, 
                                    const bool isMax)
{
    char player = 'W', opponent = 'B';
    if (!isMax) {std::swap(player, opponent);}
    
    // check below
    int i = row+1;
    bool found = false;
    for (; i<SIZE; ++i) {
        if (table[i][col] == player) {
            if (i - row > 1) {
                found = true;
                break;
            }
            else {break;}
        }
        else if (table[i][col] == ' ') {break;}
    }
    if (found) {
        for (int j=row+1; j<i; ++j) {
            table[j][col] = player;
        }
    }
    
    // check above
    i = row-1;
    found = false;
    for (; i>=0; --i) {
        if (table[i][col] == player) {
            if (row - i > 1) {
                found = true;
                break;
            }
            else {break;}
        }
        else if (table[i][col] == ' ') {break;}
    }
    if (found) {
        for (int j=row-1; j>i; --j) {
            table[j][col] = player;
        }
    }
}

void MainWindow::flip_pieces_on_diag(std::vector<std::vector<char>>& table,
                                     const int row, 
                                     const int col, 
                                     const bool isMax)
{
    char player = 'W', opponent = 'B';
    if (!isMax) {std::swap(player, opponent);}
    
    // check lower right diagonal
    bool found = false;
    int i = col+1;
    for (int j=1; i<SIZE; ++i, ++j) {
        if (is_safe_coord(row+j, i)) {
            if (table[row+j][i] == player) {
                if (i - col > 1) {
                    found = true;
                    break;
                }
                else {break;}
            }
            else if (table[row+j][i] == ' ') {break;}
        }
        else {break;}
    }
    if (found) {
        for (int k=col+1, j=1; k<i; ++k, ++j) {
            table[row+j][k] = player;
        }
    }
    
    // check upper right diagonal
    found = false;
    i = col + 1;
    for (int j=1; i<SIZE; ++i, ++j) {
        if (is_safe_coord(row-j, i)) {
            if (table[row-j][i] == player) {
                if (i - col > 1) {
                    found = true;
                    break;
                }
                else {break;}
            }
            else if (table[row-j][i] == ' ') {break;}
        }
        else {break;}
    }
    if (found) {
        for (int k=col+1, j=1; k<i; ++k, ++j) {
            table[row-j][k] = player;
        }
    }
    
    // check upper left diagonal
    found = false;
    i = col-1;
    for (int j=1; i>=0; --i, ++j) {
        if (is_safe_coord(row-j, i)) {
            if (table[row-j][i] == player) {
                if (col - i > 1) {
                    found = true;
                    break;
                }
                else {break;}
            }
            else if (table[row-j][i] == ' ') {break;}
        }
        else {break;}
    }
    if (found) {
        for (int k=col-1, j=1; k>i; --k, ++j) {
            table[row-j][k] = player;
        }
    }
    
    // check lower left diagonal
    found = false;
    i = col-1;
    for (int j=1; i>=0; --i, ++j) {
        if (is_safe_coord(row+j, i)) { 
            if (table[row+j][i] == player) {
                if (col - i > 1) {
                    found = true;
                    break;
                }
                else {break;}
            }
            else if (table[row+j][i] == ' ') {break;}
        }
        else {break;}
    }
    if (found) {
        for (int k=col-1, j=1; k>i; --k, ++j) {
            table[row+j][k] = player;
        }
    }
}

void MainWindow::make_move(std::vector<std::vector<char>>& table,
                           const int row, 
                           const int col, 
                           const bool isMax)
{   
    char player = 'W', opponent = 'B';
    if (!isMax) {std::swap(player, opponent);}
    
    // Put piece on its place
    table[row][col] = player;
    
    // Flip pieces on the same row
    flip_pieces_on_row(table, row, col, isMax);
    
    // Flip pieces on the same col
    flip_pieces_on_col(table, row, col, isMax);
    
    // Flip pieces on same diagonals
    flip_pieces_on_diag(table, row, col, isMax);
}

bool MainWindow::has_moves_available(const std::vector<std::vector<char>>& table,
                                     const bool isMax) const noexcept
{
    for (int i=0; i<SIZE; ++i) {
        for (int j=0; j<SIZE; ++j) {
            if (table[i][j] == ' ') {
                if (is_valid_move(table, i, j, isMax)) {
                    return true;
                }
            }
        }
    }
    
    return false;
}

int MainWindow::num_moves_available(const std::vector<std::vector<char>>& table,
                                    const bool isMax) const noexcept
{
    int res = 0;
    
    for (int i=0; i<SIZE; ++i)
        for (int j=0; j<SIZE; ++j)
            if (table[i][j] == ' ')
                if (is_valid_move(table, i, j, isMax))
                    res++;
                
    return res;
}                   

std::vector<std::pair<int,int>> MainWindow::all_moves_available(const std::vector<std::vector<char>>& table,
                                                                const bool isMax) const
{
    std::vector<std::pair<int,int>> result;
    
    for (int i=0; i<SIZE; ++i) {
        for (int j=0; j<SIZE; ++j) {
            if (table[i][j] == ' ') {
                if (is_valid_move(table, i, j, isMax)) {
                    result.emplace_back(i, j);
                }
            }
        }
    }
    
    return result;
}


void MainWindow::newGame()
{
    static QIcon ic = QIcon();
    
    for (int i=0; i<SIZE; ++i) {
        for (int j=0; j<SIZE; ++j) {
            board[i][j] = ' ';
            btn_storage[i][j]->setIcon(ic);
            btn_storage[i][j]->setEnabled(true);
        }
    }
    
    board[3][3] = 'W';
    btn_storage[3][3]->setIcon(white);
    btn_storage[3][3]->setEnabled(false);
    board[4][4] = 'W';
    btn_storage[4][4]->setIcon(white);
    btn_storage[4][4]->setEnabled(false);
    board[3][4] = 'B';
    btn_storage[3][4]->setIcon(black);
    btn_storage[3][4]->setEnabled(false);
    board[4][3] = 'B';
    btn_storage[4][3]->setIcon(black);
    btn_storage[4][3]->setEnabled(false);
    
    update_scores();
    update_status_bar();
}

void MainWindow::set_beginner_level()
{
    level = Level::beginner;
    newGame();
}

void MainWindow::set_intermediate_level()
{
    level = Level::intermediate;
    newGame();
}

void MainWindow::set_expert_level()
{
    level = Level::expert;
    newGame();
}

inline void MainWindow::block_all_cells()
{
    for (auto& row: btn_storage) {
        for (auto& btn: row) {
            btn->setEnabled(false);
        }
    }
}

inline void MainWindow::update_scores() noexcept
{
    my_score = 0;
    computer_score = 0;
    
    for (auto& row: board) {
        for (auto& cell: row) {
            if (cell == 'B') {my_score++;}
            else if (cell == 'W') {computer_score++;}
        }
    }
}

inline void MainWindow::update_status_bar()
{
    QString lev;
    switch (level) {
        case Level::beginner:
            lev = "Beginner";
            break;
        case Level::intermediate:
            lev = "Intermediate";
            break;
        case Level::expert:
            lev = "Expert";
            break;
        default:
            lev = "Invalid";
            break;
    }
    
    lab.setText("Level: " + lev + "      " +
                "Player: " + QString::number(my_score)+
                "    Computer: " + QString::number(computer_score)+ "   ");
}

bool MainWindow::check_end() const noexcept
{
    if (my_score == 0) {
        return true;
    }
    if (computer_score == 0) {
        return true;
    }
    
    // If all cells are filled its endgame also
    bool endgame = true;
    for (auto& row: board) {
        for (auto& cell: row) {
            if (cell == ' ') { // there is some moves left
                endgame = false;
                break;
            }
        }
    }
    
    return endgame;
}

/* This just dispatches to the suitable function according to
 * the difficulty level. Could have used inheritance and polymorphism
 * but found it simpler this way */
std::pair<int,int> MainWindow::computer_move(const bool isMax)
{
    switch(level) {
        case Level::beginner:
            return computer_move_beginner(isMax);
        case Level::intermediate:
            return computer_move_intermediate(isMax);
        case Level::expert:
            return computer_move_expert(isMax);
        default:
            return computer_move_expert(isMax);
    }
}

/* This is the begginer level function, which returns a random move
 * from the set of moves available */
std::pair<int,int> MainWindow::computer_move_beginner(const bool isMax)
{
    auto moves_choice = all_moves_available(board, isMax);
    std::uniform_int_distribution<int> dist (0, moves_choice.size()-1);
    return moves_choice[dist(engine)];
}

std::pair<int,int> MainWindow::computer_move_intermediate(const bool isMax)
{
    double bestval = -100000;
    int row = -1, col = -1;
    
    auto possible_moves = all_moves_available(board, isMax);
    std::vector<std::vector<char>> tmp (board); //copy
    
    for (auto& p: possible_moves) {
        // make the move
        make_move(tmp, p.first, p.second, isMax);
        
        // compute evaluation function for this move
        double tmp_val = minimax(tmp, 0, 2, !isMax);
        
        // undo the move
        tmp = board;
        
        if (tmp_val > bestval) {
            bestval = tmp_val;
            row = p.first;
            col = p.second;
        }
    }
    return std::pair<int,int>(row, col);
}

std::pair<int,int> MainWindow::computer_move_expert(const bool isMax) 
{
    double bestval = -100000;
    int row = -1, col = -1;
    
    auto possible_moves = all_moves_available(board, isMax);
    std::vector<std::vector<char>> tmp (board); //copy
    
    for (auto& p: possible_moves) {
        // make the move
        make_move(tmp, p.first, p.second, isMax);
        
        // compute evaluation function for this move
        double tmp_val = minimax(tmp, 0, 4, !isMax);
        
        // undo the move
        tmp = board;
        
        if (tmp_val > bestval) {
            bestval = tmp_val;
            row = p.first;
            col = p.second;
        }
    }
    return std::pair<int,int>(row, col);
}


/* Source: https://github.com/kartikkukreja/blog-codes/blob/master/src/Heuristic%20Function%20for%20Reversi%20(Othello).cpp */
double MainWindow::dynamic_heuristic_evaluation_function(const std::vector<std::vector<char>>& grid, 
                                                         const bool isMax)
{
    char my_color = 'W', opp_color = 'B';
    if (!isMax) {std::swap(my_color, opp_color);}
    
    int my_tiles = 0, opp_tiles = 0, i, j, k, my_front_tiles = 0, opp_front_tiles = 0, x, y;
    double p = 0, c = 0, l = 0, m = 0, f = 0, d = 0;

    static const int X1[] = {-1, -1, 0, 1, 1, 1, 0, -1};
    static const int Y1[] = {0, 1, 1, 1, 0, -1, -1, -1};
    static const int V[8][8] = {{20, -3, 11, 8, 8, 11, -3, 20},
                                {-3, -7, -4, 1, 1, -4, -7, -3},
                                {11, -4, 2, 2, 2, 2, -4, 11},
                                {8, 1, 2, -3, -3, 2, 1, 8},
                                {8, 1, 2, -3, -3, 2, 1, 8},
                                {11, -4, 2, 2, 2, 2, -4, 11},
                                {-3, -7, -4, 1, 1, -4, -7, -3},
                                {20, -3, 11, 8, 8, 11, -3, 20}};

    // Piece difference, frontier disks and disk squares
    for(i=0; i<SIZE; i++)
        for(j=0; j<SIZE; j++)  {
            if(grid[i][j] == my_color)  {
                d += V[i][j];
                my_tiles++;
            } else if(grid[i][j] == opp_color)  {
                d -= V[i][j];
                opp_tiles++;
            }
            if(grid[i][j] != '-')   {
                for(k=0; k<SIZE; k++)  {
                    x = i + X1[k]; y = j + Y1[k];
                    if(x >= 0 && x < SIZE && y >= 0 && y < SIZE && grid[x][y] == ' ') {
                        if(grid[i][j] == my_color)  my_front_tiles++;
                        else opp_front_tiles++;
                        break;
                    }
                }
            }
        }
    if(my_tiles > opp_tiles)
        p = (100.0 * my_tiles)/(my_tiles + opp_tiles);
    else if(my_tiles < opp_tiles)
        p = -(100.0 * opp_tiles)/(my_tiles + opp_tiles);
    else p = 0;

    if(my_front_tiles > opp_front_tiles)
        f = -(100.0 * my_front_tiles)/(my_front_tiles + opp_front_tiles);
    else if(my_front_tiles < opp_front_tiles)
        f = (100.0 * opp_front_tiles)/(my_front_tiles + opp_front_tiles);
    else f = 0;

    // Corner occupancy
    my_tiles = opp_tiles = 0;
    if(grid[0][0] == my_color) my_tiles++;
    else if(grid[0][0] == opp_color) opp_tiles++;
    if(grid[0][7] == my_color) my_tiles++;
    else if(grid[0][7] == opp_color) opp_tiles++;
    if(grid[7][0] == my_color) my_tiles++;
    else if(grid[7][0] == opp_color) opp_tiles++;
    if(grid[7][7] == my_color) my_tiles++;
    else if(grid[7][7] == opp_color) opp_tiles++;
    c = 25 * (my_tiles - opp_tiles);

    // Corner closeness
    my_tiles = opp_tiles = 0;
    if(grid[0][0] == ' ')   {
        if(grid[0][1] == my_color) my_tiles++;
        else if(grid[0][1] == opp_color) opp_tiles++;
        if(grid[1][1] == my_color) my_tiles++;
        else if(grid[1][1] == opp_color) opp_tiles++;
        if(grid[1][0] == my_color) my_tiles++;
        else if(grid[1][0] == opp_color) opp_tiles++;
    }
    if(grid[0][7] == ' ')   {
        if(grid[0][6] == my_color) my_tiles++;
        else if(grid[0][6] == opp_color) opp_tiles++;
        if(grid[1][6] == my_color) my_tiles++;
        else if(grid[1][6] == opp_color) opp_tiles++;
        if(grid[1][7] == my_color) my_tiles++;
        else if(grid[1][7] == opp_color) opp_tiles++;
    }
    if(grid[7][0] == ' ')   {
        if(grid[7][1] == my_color) my_tiles++;
        else if(grid[7][1] == opp_color) opp_tiles++;
        if(grid[6][1] == my_color) my_tiles++;
        else if(grid[6][1] == opp_color) opp_tiles++;
        if(grid[6][0] == my_color) my_tiles++;
        else if(grid[6][0] == opp_color) opp_tiles++;
    }
    if(grid[7][7] == ' ')   {
        if(grid[6][7] == my_color) my_tiles++;
        else if(grid[6][7] == opp_color) opp_tiles++;
        if(grid[6][6] == my_color) my_tiles++;
        else if(grid[6][6] == opp_color) opp_tiles++;
        if(grid[7][6] == my_color) my_tiles++;
        else if(grid[7][6] == opp_color) opp_tiles++;
    }
    l = -12.5 * (my_tiles - opp_tiles);

    // Mobility
    my_tiles = num_moves_available(grid, isMax);
    opp_tiles = num_moves_available(grid, !isMax);
    if(my_tiles > opp_tiles)
        m = (100.0 * my_tiles)/(my_tiles + opp_tiles);
    else if(my_tiles < opp_tiles)
        m = -(100.0 * opp_tiles)/(my_tiles + opp_tiles);
    else m = 0;

    // final weighted score
    double score = (10 * p) + (801.724 * c) + (382.026 * l) + (78.922 * m) + (74.396 * f) + (10 * d);
    return score;
}

double MainWindow::minimax(std::vector<std::vector<char>>& table,
                           int depth,
                           int max_depth,
                           bool isMax)
{
    // base cases
    if (depth == max_depth) {
        return dynamic_heuristic_evaluation_function(table, true);
    }
    
    char player = 'W', opponent = 'B';
    
    int pl_cnt = 0, opp_cnt = 0;
    bool endgame = true;
    for (auto& row: table) {
        for (auto& cell: row) {
            if (cell == player) pl_cnt++;
            else if (cell == opponent) opp_cnt++;
            else if (cell == ' ') endgame = false;
        }
    }
    
    if (endgame || pl_cnt == 0 || opp_cnt == 0) {
        if (pl_cnt > opp_cnt) return 100000;
        else if (pl_cnt < opp_cnt) return -100000;
        else {return 0;} // draw
    }
    
    std::vector<std::vector<char>> tmp (table); // copy
    
    if (isMax) { // Maximizer's move
        double best = -100000;
        auto moves_av = all_moves_available(tmp, true);
        if (moves_av.size() == 0) {
            return std::max(best, minimax(tmp,depth+1, max_depth, !isMax));
        }
        
        for (auto& mv: moves_av) {
            // make the move
            make_move(tmp, mv.first, mv.second, true);
            
            // Call minimax recursively and choose the maximum value
            best = std::max(best, minimax(tmp,depth+1, max_depth, !isMax));
            
            // Undo the move
            tmp = table;
        }
        return best;
    }
    
    else { // Minimizer's move
        double best = 100000;
        auto moves_av = all_moves_available(tmp, false);
        if (moves_av.size() == 0) {
            return std::min(best, minimax(tmp,depth+1, max_depth, !isMax));
        }
        
        for (auto& mv: moves_av) {
            // make the move
            make_move(tmp, mv.first, mv.second, false);
            
            // Call minimax recursively and choose the maximum value
            best = std::min(best, minimax(tmp,depth+1, max_depth, !isMax));
            
            // Undo the move
            tmp = table;
        }
        return best;
    }
}

void MainWindow::hint()
{
    auto coord = computer_move_intermediate(false);
    QString msg = "Try coordinates ("+QString::number(coord.first)+","+QString::number(coord.second)+")";
    QMessageBox::information(this, "Hint", msg);
}

void MainWindow::about()
{
    QMessageBox::information(this, "About",
      "Reversi version 0.1\nAuthor: Fernando B. Giannasi\nmar/2018");
}