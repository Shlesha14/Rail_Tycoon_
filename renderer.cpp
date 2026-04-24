/*
 * renderer.cpp
 * ------------
 * All SFML drawing code.
 *
 * Public functions:
 *   drawLine()  - draws a line using a thin rotated rectangle
 *   drawText()  - draws a text string at a given position
 *   drawFrame() - draws the complete game screen once per frame
 *
 * drawFrame() draw order (back to front):
 *   1. Zone background bands + grid
 *   2. Routes with profit labels
 *   3. Algorithm overlays (yellow Dijkstra / cyan BFS)
 *   4. Cost preview ghost line
 *   5. Train dots
 *   6. Node shapes and labels
 *   7. Right panel (title, phase, money, finance, complexity, log, controls)
 *   8. Status bar
 */

#include "rail_empire.h"
#include <sstream>
using namespace std;

// =========================================================================
//  DRAW UTILITIES
// =========================================================================

// Draws a line by using a thin rotated rectangle (SFML has no line primitive)
void drawLine(sf::RenderWindow& w,
              sf::Vector2f a, sf::Vector2f b,
              sf::Color col, float thick = 2.f){
    sf::Vector2f d   = b - a;
    float        len = hypotf(d.x, d.y);
    if(len < 0.1f) return;
    sf::RectangleShape line({len, thick});
    line.setOrigin({0.f, thick/2});
    line.setPosition(a);
    line.setRotation(sf::degrees(atan2f(d.y, d.x) * 180.f / 3.14159f));
    line.setFillColor(col);
    w.draw(line);
}

// Draws a text string at (x, y) with given font size and colour
void drawText(sf::RenderWindow& w, const string& s,
              float x, float y, int sz = 13,
              sf::Color col = sf::Color::White){
    sf::Text t(font, s, sz);
    t.setPosition({x, y});
    t.setFillColor(col);
    w.draw(t);
}

// =========================================================================
//  PANEL HELPERS
// =========================================================================

// Draws one finance block (YOUR FINANCE or AI FINANCE) with 4 metric rows
static void drawFinanceBlock(sf::RenderWindow& window,
                              const string& title, Finance& f,
                              sf::Color headerCol,
                              float& py, float PX, float PW){
    sf::RectangleShape hdr({PW, 18.f});
    hdr.setPosition({PX-2.f, py}); hdr.setFillColor({20,30,52});
    window.draw(hdr);
    drawText(window, title, PX+4.f, py+2.f, 11, headerCol);
    py += 22.f;

    struct Row { const char* label; int value; sf::Color colour; };
    int net = f.net();
    Row rows[] = {
        {"Revenue", f.revenue,   {80,220,80}},
        {"Build",   f.buildCost, {255,155,60}},
        {"Maint",   f.maint,     {255,100,100}},
        {"NET",     net,          net>=0 ? sf::Color{80,255,80} : sf::Color{255,80,80}},
    };
    for(auto& row : rows){
        bool isNet = string(row.label) == "NET";
        drawText(window, row.label, PX+4.f, py+1.f,
                 isNet?11:10,
                 isNet ? sf::Color(220,220,220) : sf::Color(150,170,200));
        drawText(window, "$"+to_string(row.value), PX+PW-52.f, py+1.f,
                 isNet?12:10, row.colour);
        py += isNet ? 16.f : 14.f;
    }
    py += 6.f;
    drawLine(window, {PX,py}, {PX+PW,py}, {40,60,90}, 1.f);
    py += 7.f;
}

// Draws the algorithm complexity reference table
static void drawComplexityTable(sf::RenderWindow& window,
                                 float& py, float PX, float PW){
    drawText(window, "ALGORITHM COMPLEXITY", PX, py, 10, {255,200,50});
    py += 14.f;

    struct CXRow { const char* algo; const char* unit; const char* big; };
    CXRow rows[] = {
        {"Kruskal MST",  "U3", "O(E log E)"},
        {"Prim MST",     "U3", "O((V+E)logV)"},
        {"Dijkstra",     "U3", "O((V+E)logV)"},
        {"Bellman-Ford", "U3", "O(V x E)"},
        {"BFS",          "U3", "O(V+E)"},
        {"Merge Sort",   "U2", "O(n log n)"},
        {"Knapsack DP",  "U4", "O(n x W)"},
        {"DSU find",     "U3", "O(a(n))"},
    };
    for(auto& cx : rows){
        if(py > 655.f) break;
        sf::RectangleShape badge({22.f,13.f});
        badge.setPosition({PX,py}); badge.setFillColor({30,55,90});
        window.draw(badge);
        drawText(window, cx.unit, PX+1.f,       py+1.f, 8, {100,180,255});
        drawText(window, cx.algo, PX+26.f,      py+1.f, 9, {180,200,175});
        drawText(window, cx.big,  PX+PW-68.f,  py+1.f, 9, {255,215,100});
        py += 14.f;
    }
    drawLine(window, {PX,py}, {PX+PW,py}, {45,70,105}, 1.f);
    py += 5.f;
}

// Draws the controls reference section pinned to the bottom of the panel
static void drawControlsSection(sf::RenderWindow& window, float PX, float PW){
    sf::RectangleShape bg({(float)PANEL, 136.f});
    bg.setPosition({(float)MAP_W, (float)(WIN_H-136)});
    bg.setFillColor({9,14,26});
    window.draw(bg);
    drawLine(window,
             {(float)MAP_W,(float)(WIN_H-136)},
             {(float)WIN_W,(float)(WIN_H-136)},
             {50,75,110}, 1.f);

    float y = WIN_H - 133.f;
    drawText(window, "CONTROLS", PX, y, 10, {100,155,220}); y += 13.f;
    drawLine(window, {PX,y}, {PX+PW,y}, {35,52,78}, 1.f); y += 4.f;

    struct KRow { const char* key; const char* desc; };
    KRow keys[] = {
        {"LClick","Select + build"},   {"RClick","Run path algo"},
        {"ENTER", "Simulate turn"},    {"SPACE", "Advance phase"},
        {"D",     "Dijkstra mode"},    {"B",     "Bellman-Ford mode"},
        {"F",     "BFS mode"},         {"C",     "BFS from node 0"},
        {"P",     "Kruskal/Prim MST"}, {"M",     "Toggle MST lines"},
        {"S",     "Toggle SPT lines"}, {"V",     "Toggle path viz"},
        {"Y",     "Toggle player"},    {"L",     "Toggle algo log"},
        {"R",     "Restart"},          {"",      ""},
    };
    for(int ki = 0; ki < 16; ki++){
        float kx = PX + (ki%2) * (PW/2.f);
        drawText(window, keys[ki].key,  kx,        y, 9, {255,195,70});
        drawText(window, keys[ki].desc, kx+28.f,   y, 9, {160,180,205});
        if(ki%2 == 1) y += 12.f;
    }
}

// =========================================================================
//  FULL FRAME RENDER
// =========================================================================
void drawFrame(sf::RenderWindow& window, const string& statusMsg){
    window.clear(sf::Color(10,15,26));

    // ---- 1. Zone backgrounds ----
    sf::RectangleShape zA({220.f,(float)WIN_H}); zA.setPosition({0,0});   zA.setFillColor({10,22,14}); window.draw(zA);
    sf::RectangleShape zB({200.f,(float)WIN_H}); zB.setPosition({220,0}); zB.setFillColor({18,18,22}); window.draw(zB);
    sf::RectangleShape zC({220.f,(float)WIN_H}); zC.setPosition({420,0}); zC.setFillColor({22,16,10}); window.draw(zC);
    sf::RectangleShape zD({(float)(MAP_W-640),(float)WIN_H}); zD.setPosition({640,0}); zD.setFillColor({10,14,26}); window.draw(zD);

    drawText(window,"FARMS",     18, 4,9,{40,120,40});
    drawText(window,"MINES",    228, 4,9,{100,100,110});
    drawText(window,"FACTORIES",428, 4,9,{140,80,30});
    drawText(window,"CITIES",   680, 4,9,{50,100,180});

    drawLine(window,{220,0},{220,(float)WIN_H},{30,45,30},1.f);
    drawLine(window,{420,0},{420,(float)WIN_H},{40,35,25},1.f);
    drawLine(window,{640,0},{640,(float)WIN_H},{25,35,55},1.f);

    for(int x=0;x<MAP_W;x+=55)
        drawLine(window,{(float)x,0},{(float)x,(float)WIN_H},{20,28,40},1.f);
    for(int y=0;y<WIN_H;y+=55)
        drawLine(window,{0,(float)y},{(float)MAP_W,(float)y},{20,28,40},1.f);

    // ---- 2. Routes ----
    for(auto& r : routes){
        sf::Color col;
        switch(r.rtype){
            case RT_MST:       col={0,210,110};   break;
            case RT_SPT:       col={180,100,255}; break;
            case RT_PLAYER:    col={70,150,255};  break;
            case RT_AI_PROFIT: col={255,70,70};   break;
        }
        if(!showMST    && r.rtype==RT_MST)    continue;
        if(!showSPT    && r.rtype==RT_SPT)    continue;
        if(!showPlayer && r.rtype==RT_PLAYER) continue;
        drawLine(window, npos(r.u), npos(r.v), col, 2.5f);

        if(r.totalProfit > 0){
            sf::Vector2f a=npos(r.u), b=npos(r.v), mid=(a+b)*0.5f;
            sf::Vector2f d=b-a; float len=hypotf(d.x,d.y);
            if(len>1.f){
                sf::Vector2f perp={-d.y/len*14.f, d.x/len*14.f};
                if(perp.y>0) perp={-perp.x,-perp.y};
                mid+=perp;
            }
            drawText(window,"$"+to_string(r.totalProfit),mid.x-10,mid.y-6,9,{255,230,90});
        }
    }

    // ---- 3a. Dijkstra / Bellman-Ford yellow overlay ----
    if(pathSrc>=0 && showViz && algoMode!=ALGO_BFS){
        for(int i=0;i<N();i++)
            if(pathPrev[i]>=0 && pathDist[i]<1e8f){
                drawLine(window, npos(pathPrev[i]), npos(i), {255,255,0,255}, 4.f);
                sf::CircleShape dot(5.f); dot.setOrigin({5.f,5.f});
                dot.setPosition(npos(i)); dot.setFillColor({255,255,0,255});
                window.draw(dot);
            }
    }

    // ---- 3b. BFS cyan overlay ----
    if(pathSrc>=0 && showViz && algoMode==ALGO_BFS){
        for(int i=0;i<N();i++)
            if((int)bfsVisited.size()>i && bfsVisited[i])
                drawLine(window, npos(pathSrc), npos(i), {100,255,255,220}, 3.f);
    }

    // ---- 4. Cost preview ghost line ----
    if(state==S_BUILD && selected>=0 && hovered<0){
        sf::Vector2f mp = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        drawLine(window, npos(selected), mp, {255,255,255,60}, 1.5f);
        float d = vdist(npos(selected), mp);
        drawText(window,"$"+to_string(max(10,(int)(d*ROUTE_COST_PER_PX))),
                 mp.x+5, mp.y-16, 11, {200,200,200});
    }

    // ---- 5. Train dots ----
    for(auto& tr : trains){
        Route& r = routes[tr.routeIdx];
        sf::Vector2f pos = npos(r.u) + (npos(r.v)-npos(r.u)) * tr.t;
        sf::CircleShape c(5.f); c.setOrigin({5.f,5.f}); c.setPosition(pos);
        c.setFillColor(cargoCol(r.cargo));
        c.setOutlineThickness(1.f); c.setOutlineColor({20,20,20});
        window.draw(c);
    }

    // ---- 6. Nodes ----
    for(int i=0;i<N();i++){
        Node& nd = nodes[i];
        bool  sel=(selected==i), hov=(hovered==i);
        float r  = NODE_R + (hov?4:0);
        sf::Color fill=nodeCol(nd);
        sf::Color outl=sel?sf::Color::Yellow:sf::Color(180,180,180);
        float ot=sel?3.f:1.f;

        if(nd.type==CITY){
            sf::CircleShape s(r); s.setOrigin({r,r}); s.setPosition(nd.pos);
            s.setFillColor(fill); s.setOutlineThickness(ot); s.setOutlineColor(outl);
            window.draw(s);
        } else if(nd.type==STATION){
            sf::RectangleShape s({r*2.2f,r*2.2f}); s.setOrigin({r*1.1f,r*1.1f});
            s.setPosition(nd.pos); s.setRotation(sf::degrees(45.f));
            s.setFillColor(fill); s.setOutlineThickness(ot); s.setOutlineColor(outl);
            window.draw(s);
        } else {
            sf::RectangleShape s({r*2,r*2}); s.setOrigin({r,r}); s.setPosition(nd.pos);
            s.setFillColor(fill); s.setOutlineThickness(ot); s.setOutlineColor(outl);
            window.draw(s);
        }

        // BFS visited ring
        if(pathSrc>=0 && showViz && algoMode==ALGO_BFS
           && (int)bfsVisited.size()>i && bfsVisited[i]){
            sf::CircleShape ring(r+4); ring.setOrigin({r+4,r+4});
            ring.setPosition(nd.pos); ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineThickness(2.f); ring.setOutlineColor({100,255,255,180});
            window.draw(ring);
        }

        // Label and stock/demand info
        string lbl; sf::Color lblCol;
        switch(nd.type){
            case CITY:      lbl="City "    +to_string(i); lblCol={160,200,255}; break;
            case FARM:      lbl="Farm "    +to_string(i); lblCol={120,220,100}; break;
            case COAL_MINE: lbl="Mine "    +to_string(i); lblCol={180,180,190}; break;
            case FACTORY:   lbl="Fact "    +to_string(i); lblCol={255,180, 80}; break;
            case STATION:   lbl="Station " +to_string(i); lblCol={200,160,255}; break;
            default:        lbl=to_string(i);             lblCol=sf::Color::White;
        }
        drawText(window, lbl, nd.pos.x+NODE_R+3, nd.pos.y-8, 11, lblCol);
        if(nd.type==CITY)
            drawText(window,"F:"+to_string(nd.demand[FOOD])+" G:"+to_string(nd.demand[GOODS]),
                     nd.pos.x+NODE_R+3, nd.pos.y+5, 9, {140,160,200});
        else if(nd.type==STATION)
            drawText(window,"Station hub", nd.pos.x+NODE_R+3, nd.pos.y+5, 9, {200,180,230});
        else
            drawText(window,"stk:"+to_string(nd.stock),
                     nd.pos.x+NODE_R+3, nd.pos.y+5, 9, {180,200,150});
    }

    // ---- 7. Right panel ----
    sf::RectangleShape panelBg({(float)PANEL,(float)WIN_H});
    panelBg.setPosition({(float)MAP_W,0}); panelBg.setFillColor({13,19,33});
    window.draw(panelBg);
    drawLine(window,{(float)MAP_W,0},{(float)MAP_W,(float)WIN_H},{50,75,110},2.f);

    const float PX=MAP_W+14.f, PW=PANEL-26.f;

    // Title bar
    sf::RectangleShape titleBg({(float)PANEL,42.f});
    titleBg.setPosition({(float)MAP_W,0.f}); titleBg.setFillColor({8,14,28});
    window.draw(titleBg);
    drawText(window,"RAIL EMPIRE",        PX, 6.f, 19,{255,210,50});
    drawText(window,"DAA Project  TCS409",PX,26.f, 10,{75,105,145});
    drawLine(window,{(float)MAP_W,42.f},{(float)WIN_W,42.f},{45,70,105},1.f);

    float py=48.f;

    // Phase pill
    const char* phStr[]={"BUILD   Your turn - build routes",
                          "DONE    Simulated - press SPACE for AI",
                          "AI DONE Press SPACE for final result",
                          "FINAL   Game over - press R to restart"};
    sf::Color phCol=(state==S_BUILD)?sf::Color{40,120,60}
                   :(state==S_PLAYER_RESULT)?sf::Color{40,80,130}
                   :(state==S_FINAL)?sf::Color{120,40,40}
                   :sf::Color{100,60,20};
    sf::RectangleShape phPill({PW,22.f});
    phPill.setPosition({PX-2.f,py}); phPill.setFillColor(phCol);
    window.draw(phPill);
    drawText(window,phStr[(int)state],PX+4.f,py+4.f,11,{230,255,230});
    py+=26.f;

    // Money row
    sf::RectangleShape monBg({PW,20.f});
    monBg.setPosition({PX-2.f,py}); monBg.setFillColor({18,28,48});
    window.draw(monBg);
    drawText(window,"YOU  $"+to_string(playerBudget),PX+4.f,py+3.f,11,
             playerBudget>500?sf::Color(90,255,90):sf::Color(255,90,90));
    drawText(window,"AI  $"+to_string(aiBudget),PX+158.f,py+3.f,11,{210,140,140});
    py+=26.f;
    drawLine(window,{PX,py},{PX+PW,py},{45,70,105},1.f); py+=7.f;

    drawFinanceBlock(window,"YOUR FINANCE",pFin,{100,170,255},py,PX,PW);
    drawFinanceBlock(window,"AI FINANCE",  aFin,{255,110,110},py,PX,PW);

    // Winner banner
    if(state==S_FINAL){
        bool win=pFin.net()>aFin.net();
        sf::RectangleShape wb({PW,28.f}); wb.setPosition({PX-2.f,py});
        wb.setFillColor(win?sf::Color{30,75,20}:sf::Color{75,20,20});
        window.draw(wb);
        string msg=win?"YOU WIN!  +"+to_string(pFin.net()-aFin.net())
                      :"AI WINS   +"+to_string(aFin.net()-pFin.net());
        drawText(window,msg,PX+6.f,py+6.f,13,win?sf::Color(120,255,90):sf::Color(255,90,90));
        py+=34.f;
        drawLine(window,{PX,py},{PX+PW,py},{45,70,105},1.f); py+=7.f;
    }

    drawComplexityTable(window, py, PX, PW);

    // Algo log
    if(showAlgoLog && !algoLog.empty()){
        drawText(window,"ALGO LOG  (L=hide)",PX,py,10,{255,200,50}); py+=12.f;
        stringstream ss(algoLog); string line;
        while(getline(ss,line)){
            if(py>=738.f) break;
            drawText(window,line,PX,py,9,{185,210,165}); py+=11.f;
        }
    }

    drawControlsSection(window, PX, PW);

    // ---- 8. Status bar ----
    sf::RectangleShape stBar({(float)MAP_W,20.f});
    stBar.setPosition({0,(float)(WIN_H-20)}); stBar.setFillColor({8,14,28});
    window.draw(stBar);
    drawLine(window,{0,(float)(WIN_H-20)},{(float)MAP_W,(float)(WIN_H-20)},{25,40,65},1.f);
    drawText(window,statusMsg,6,(float)(WIN_H-17),10,{140,175,220});

    window.display();
}
