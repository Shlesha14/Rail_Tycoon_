/*
 * main.cpp
 * --------
 * Entry point. Responsible only for:
 *   - Window creation and font loading
 *   - The main 60fps game loop
 *   - Mouse and keyboard event handling
 *   - Train animation update
 *   - Calling drawFrame() each frame
 *
 * All game logic lives in routes.cpp and ai.cpp.
 * All algorithms live in algorithms.cpp.
 * All drawing lives in renderer.cpp.
 */

#include "rail_empire.h"
using namespace std;

// Forward declarations - implemented in other .cpp files
void generateMap();
void runAI();
void simulate(bool isAI);
void runDijkstra(int src);
void runBellmanFord(int src);
void runBFS(int src);
bool addRoute(int u, int v, bool isAI, RouteType rt);
void drawFrame(sf::RenderWindow& window, const string& statusMsg);

int main(){
    sf::RenderWindow window(
        sf::VideoMode({WIN_W, WIN_H}),
        "Rail Empire - DAA TCS409"
    );
    window.setFramerateLimit(60);

    // Load font - try local directory first, fall back to system path
    if(!font.openFromFile("arial.ttf"))
        font.openFromFile("/System/Library/Fonts/Supplemental/Arial.ttf");

    generateMap();

    string statusMsg = "LClick: select node to build route  |  "
                       "RClick: run path algo  |  "
                       "ENTER: simulate your turn";

    // =========================================================================
    //  MAIN GAME LOOP (60 fps)
    // =========================================================================
    while(window.isOpen()){

        // ---- Hover detection ----
        sf::Vector2f mp = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        hovered = -1;
        for(int i = 0; i < N(); i++)
            if(vdist(mp, npos(i)) < NODE_R+5){ hovered = i; break; }

        // ---- Event handling ----
        while(auto ev = window.pollEvent()){

            if(ev->is<sf::Event::Closed>()) window.close();

            // -- Mouse clicks --
            if(auto* mb = ev->getIf<sf::Event::MouseButtonPressed>()){

                // Right-click on a node: run the active path algorithm
                if(mb->button == sf::Mouse::Button::Right && hovered >= 0){
                    switch(algoMode){
                        case ALGO_DIJKSTRA: runDijkstra(hovered);    break;
                        case ALGO_BELLMAN:  runBellmanFord(hovered); break;
                        case ALGO_BFS:      runBFS(hovered);         break;
                    }
                }

                // Left-click in BUILD phase: first click selects, second click builds
                if(mb->button == sf::Mouse::Button::Left && state == S_BUILD){
                    if(hovered >= 0){
                        if(selected < 0){
                            selected  = hovered;
                            statusMsg = "Node " + to_string(selected)
                                      + " selected - click destination";
                        } else {
                            if(selected != hovered){
                                float d    = vdist(npos(selected), npos(hovered));
                                int   cost = max(10, (int)(d * ROUTE_COST_PER_PX));
                                if(playerBudget >= cost){
                                    addRoute(selected, hovered, false, RT_PLAYER);
                                    statusMsg = "Route built! Cost=$" + to_string(cost)
                                              + "  Remaining=$" + to_string(playerBudget);
                                } else {
                                    statusMsg = "Not enough money! Need $" + to_string(cost);
                                }
                            }
                            selected = -1;
                        }
                    }
                }
            }

            // -- Keyboard --
            if(auto* kp = ev->getIf<sf::Event::KeyPressed>()){
                using K = sf::Keyboard::Key;
                auto key = kp->code;

                // Path algorithm mode (D/B/F - then right-click a node to run)
                if(key==K::D){ algoMode=ALGO_DIJKSTRA;
                    statusMsg="Dijkstra ready - right-click any node to run"; }
                if(key==K::B){ algoMode=ALGO_BELLMAN;
                    statusMsg="Bellman-Ford ready - right-click any node to run"; }
                if(key==K::F){ algoMode=ALGO_BFS;
                    statusMsg="BFS ready - right-click any node to run"; }

                // C: run BFS connectivity check from node 0 immediately
                if(key==K::C){
                    algoMode=ALGO_BFS; runBFS(0);
                    statusMsg="BFS from node 0: cyan rings = reachable nodes";
                }

                // P: toggle AI MST between Kruskal and Prim
                if(key==K::P){ usePrim=!usePrim;
                    statusMsg="AI MST: "+(usePrim?string("PRIM"):string("KRUSKAL")); }

                // Display toggles
                if(key==K::M){ showMST=!showMST;
                    statusMsg=showMST?"MST routes: VISIBLE":"MST routes: HIDDEN"; }
                if(key==K::S)   showSPT    = !showSPT;
                if(key==K::V)   showViz    = !showViz;
                if(key==K::Y){ showPlayer=!showPlayer;
                    statusMsg=showPlayer?"Player routes: VISIBLE":"Player routes: HIDDEN"; }
                if(key==K::L){ showAlgoLog=!showAlgoLog;
                    statusMsg=showAlgoLog?"Algo log: VISIBLE":"Algo log: HIDDEN"; }

                // ENTER: simulate the player's 600-tick turn
                if(key==K::Enter && state==S_BUILD){
                    simulate(false);
                    state     = S_PLAYER_RESULT;
                    statusMsg = "Your results ready. Press SPACE for AI turn.";
                }

                // SPACE: advance game phase
                if(key==K::Space){
                    if(state==S_PLAYER_RESULT){
                        runAI(); simulate(true);
                        state     = S_AI_RESULT;
                        statusMsg = "AI done. Press SPACE for final result.";
                    } else if(state==S_AI_RESULT){
                        state     = S_FINAL;
                        statusMsg = pFin.net()>aFin.net()
                                    ? "YOU WIN!  R=restart"
                                    : "AI wins.   R=restart";
                    }
                }

                // R: restart (only allowed in FINAL phase)
                if(key==K::R && state==S_FINAL){
                    pFin={}; aFin={};
                    playerBudget=2500; aiBudget=2500;
                    selected=-1; pathSrc=-1;
                    state=S_BUILD; algoLog="";
                    statusMsg="New game. Build your network.";
                    generateMap();
                }
            }
        }

        // ---- Train animation (60 fps) ----
        // Each train bounces back and forth between its two endpoint nodes.
        // Speed is inversely proportional to route length - longer = slower.
        for(auto& tr : trains){
            Route& r   = routes[tr.routeIdx];
            float  spd = max(0.003f, 1.5f / r.len);
            if(tr.forward){
                tr.t += spd;
                if(tr.t >= 1.f){ tr.t = 1.f; tr.forward = false; }
            } else {
                tr.t -= spd;
                if(tr.t <= 0.f){ tr.t = 0.f; tr.forward = true; }
            }
        }

        // ---- Render ----
        drawFrame(window, statusMsg);
    }

    return 0;
}
