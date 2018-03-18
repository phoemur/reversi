#ifndef REVERSI_MAIN_HEADER
#define REVERSI_MAIN_HEADER

#include <QMainWindow>
#include <QSignalMapper>
#include <QString>
#include <QIcon>
#include <QPushButton>
#include <QLabel>
#include <memory>
#include <random>
#include <utility>
#include <vector>

#include "ui_MainWindow.h"

enum class Level {
    beginner=0, intermediate, expert
};

namespace Ui {
    class MainWindow; // forward declaration
}

class MainWindow final : public QMainWindow {
    Q_OBJECT
    
    std::unique_ptr<Ui::MainWindow> ui;
    static const int SIZE = 8;
    QSignalMapper* signalMapper;
    QIcon black, white;
    
    std::vector<std::vector<char>> board;
    std::vector<std::vector<QPushButton*>> btn_storage;
    
    Level level = Level::intermediate;
    
    int my_score = 0, computer_score = 0;
    QLabel lab {QString("")};
    
    std::random_device seeder {};
    std::mt19937 engine;

public:
    MainWindow(QMainWindow* parent = nullptr);
    
    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
    MainWindow(MainWindow&&) = delete;
    MainWindow& operator=(MainWindow&&) = delete;
    ~MainWindow() = default;
    
private slots:
    void buttonClicked(QString);
    void newGame();
    void set_beginner_level();
    void set_intermediate_level();
    void set_expert_level();
    void hint();
    void about();
    
private:
    bool is_safe_coord(const int row, const int col) const noexcept;
    
    bool valid_move_on_row(const std::vector<std::vector<char>>& table, 
                           const int row, 
                           const int col, 
                           const bool isMax) const noexcept;
 
    bool valid_move_on_col(const std::vector<std::vector<char>>& table, 
                           const int row,
                           const int col,
                           const bool isMax) const noexcept;
                           
    bool valid_move_on_diag(const std::vector<std::vector<char>>& table,
                            const int row,
                            const int col,
                            const bool isMax) const noexcept;
                            
    bool is_valid_move(const std::vector<std::vector<char>>& table,
                       const int row,
                       const int col,
                       const bool isMax) const noexcept;
    
    void update_icons();
    
    void flip_pieces_on_row(std::vector<std::vector<char>>& table,
                            const int row,
                            const int col,
                            const bool isMax);
    
    void flip_pieces_on_col(std::vector<std::vector<char>>& table,
                            const int row,
                            const int col,
                            const bool isMax);
    
    void flip_pieces_on_diag(std::vector<std::vector<char>>& table,
                             const int row,
                             const int col,
                             const bool isMax);
    
    void make_move(std::vector<std::vector<char>>& table,
                   const int row,
                   const int col,
                   const bool isMax);
    
    bool has_moves_available(const std::vector<std::vector<char>>& table,
                             const bool isMax) const noexcept;
                             
    int num_moves_available(const std::vector<std::vector<char>>& table,
                            const bool isMax) const noexcept;
                            
    std::vector<std::pair<int,int>> all_moves_available(const std::vector<std::vector<char>>& table,
                                                        const bool isMax) const;
    
    void block_all_cells();
    void update_scores() noexcept;
    void update_status_bar();
    bool check_end() const noexcept;
    
    std::pair<int,int> computer_move(const bool isMax);
    std::pair<int,int> computer_move_beginner(const bool isMax);
    std::pair<int,int> computer_move_intermediate(const bool isMax);
    std::pair<int,int> computer_move_expert(const bool isMax);
    
    double dynamic_heuristic_evaluation_function(const std::vector<std::vector<char>>& table, 
                                                 const bool isMax);
    
    double minimax(std::vector<std::vector<char>>& table,
                   int depth,
                   int max_depth,
                   bool isMax);

}; // class MainWindow


#endif // REVERSI_MAIN_HEADER