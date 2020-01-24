#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>
#include <cmath>
#include <random>
#include <string>
#include <vector>
#include <utility>

enum class Orientation {TOP,RIGHT};
  
std::pair<int,int> seek_parent(int act, std::vector<int> &v){
    int p = v[act];
    int old = act;
    int depth = 0;
    while ( p != act ){
      ++depth;
      act = p;
      p = v[act];
    }
    v[old] = act;
    return std::make_pair(act,depth);
};

void union_( int a , int b , std::vector<int> &v){
    std::pair<int,int> a_p , b_p;
    a_p = seek_parent(a,v);
    b_p = seek_parent(b,v);

    if(a_p.first != b_p.first){
      if(a_p.second > b_p.second){
       	v[b_p.first] = a_p.first;
      }
      else{
	v[a_p.first] = b_p.first;
      }
    }
};

typedef std::mt19937 RNG;
uint32_t seed_val = std::time(0);//other seed possible...
RNG rng;
void initialize(){
  rng.seed(seed_val);
}
std::normal_distribution<double> normal_dist(3.0, 1.0);
std::uniform_int_distribution<uint32_t> uint_dist(0,10000);

struct Edge {
  int x1{};
  int x2{};
  int y1{};
  int y2{};
  double weight{};
  bool used = 0;
  void set(int x, int y , Orientation orientation, int X, int Y) {
    x1 = x;
    y1 = y;
    if(orientation == Orientation::RIGHT){
      x2 = x+1;
      y2 = y; 
    }
    if(orientation == Orientation::TOP) {
      x2 = x;
      y2 = y+1;
    }
    weightometer(x,y,X,Y);
  }
  void weightometer(int x, int y, int X, int Y) {
    weight = std::exp(-((std::pow(x-X/2.0,2.0)+std::pow(y-Y/2.0,2.0))/100.0)) + normal_dist(rng);
  }
  bool operator<(Edge other) { return weight < other.weight;}
};


std::vector<Edge> make_labyrinth(int X, int Y) {
  int number_of_edges = (X-1)*Y+X*(Y-1);
  std::vector<Edge> edges(number_of_edges);
    { int i = 0;
      for(int x = 0; x < X-1; ++x) {
	for(int y = 0; y < Y-1; ++y) {
	  edges[i].set(x,y, Orientation::TOP, X , Y);
	  ++i;
	  edges[i].set(x,y,Orientation::RIGHT, X, Y);
	  ++i;
	}
	edges[i].set(x,Y-1,Orientation::RIGHT, X, Y);
	++i;
      }
      for(int y = 0; y < Y-1; ++y) {
	edges[i].set(X-1,y,Orientation::TOP, X, Y);
	++i;
      }
    }
    std::sort(edges.begin(),edges.end());
    std::vector<int> union_finder(X*Y);
    std::vector<int> open_sides(X*Y,0);
    std::iota(union_finder.begin(),union_finder.end(),0);
    {
      int number_of_unions = 0;
      int a , b;
      int a_parent, b_parent ,a_depth, b_depth;
      for(auto &e : edges) {
	a = e.y1*X + e.x1;
	b = e.y2*X + e.x2;
	auto [a_parent,a_depth] = seek_parent(a,union_finder);
	auto [b_parent,b_depth] = seek_parent(b,union_finder);
	if(open_sides[a]<3 and open_sides[b]<3 and a_parent != b_parent) {
	  e.used = 1;
	  //union_(a,b,union_finder);
	  if(a_depth > b_depth) {union_finder[b_parent] = a_parent; }
	  else {union_finder[a_parent] = b_parent; }
	  ++open_sides[a];
	  ++open_sides[b];
	  ++number_of_unions;
	}
	else {
	  if(number_of_unions >= X*Y-1) {
	    std::cout << "all conected\n";
	    break;
	  }
	}
      }
      
    }
    return edges;
}

template <typename T>
T calculate_svg_coordinates_from( T input_coordinate) {
  return input_coordinate + 5;
}

void insert_svg_line(std::fstream& s, int x1 , int y1, int x2, int y2, std::string indent = "    " ) {
  auto o = [] (auto input_coordinate) {return calculate_svg_coordinates_from(input_coordinate);};
  s << indent << R"(<line x1=")" << o(x1) << R"(" y1=")" << o(y1) << R"(" x2=")" << o(x2) << R"(" y2=")" << o(y2) << R"("/>)" << '\n';
}

void edges_to_svg(std::vector<Edge> & edges, std::fstream & s, int X , int Y) {
  auto o = [] (auto input_coordinate) {return calculate_svg_coordinates_from(input_coordinate);};
  
  s << "<?xml version=\"1.0\" standalone=\"yes\"?>\n";
  s << R"(
<svg width=")" << 10*X << R"(" height=")" << 10*Y << R"(" viewBox="0 0 )" << o(o(X)) << ' ' <<  o(o(Y)) << R"(" version="1.1"
   xmlns="http://www.w3.org/2000/svg">
<desc>labyrinth of size )" << o(X) << R"(x)" << o(Y) << R"(</desc>
  <rect x="0" y="0" width=")" << o(o(X)) << R"(" height=")" << o(o(Y)) << R"("
      fill="white" />
  <g stroke="black"  stroke-width="0.1"> )" << '\n';
  insert_svg_line(s,0,Y-1,0,0);
  insert_svg_line(s,0,0,X,0);
  insert_svg_line(s,X,1,X,Y);
  insert_svg_line(s,X,Y,0,Y);
  
  for(auto& e : edges) {
    if(e.used) continue;
    if(e.x1 == e.x2) {//horizontal line
      insert_svg_line(s,e.x1,e.y2, e.x1+1,e.y2);
    }
    else {
      insert_svg_line(s,e.x2,e.y1,e.x2,e.y2+1);
    }
  }
  s << "</g>" << "\n";
  s << "</svg>\n";
}


void edges_to_gp(std::vector<Edge> & edges, std::fstream & s, int X , int Y) {

  s << "set terminal epslatex size 10,10 standalone\n";
  s << "set output 'last_labyrinth.tex'\n";
  s << "set key off\n";
  s << "unset ytics\n";
  s << "unset xtics\n";
  s << "set yrange [-2:" << Y+1 << "]\n";
  s << "set xrange [-2:" << X+1 << "]\n";
  s << "set arrow from -0.5," << Y-1.5 << " to -0.5," << -0.5 << " nohead\n";
  s << "set arrow from " << X-0.5 << "," << Y-0.5 << " to " << X-0.5 << "," << 1-0.5 << "nohead\n";
  s << "set arrow from " << X-0.5 << "," << Y-0.5 << " to " << 0-0.5 << "," << Y-0.5 << "nohead\n";
  s << "set arrow from -0.5,-0.5 to " << X-0.5 << "," << 0-0.5 << " nohead\n";
  
  for(auto& e : edges) {
    if (e.used) continue;
    if(e.x1 == e.x2){//horizontal line
      s << "set arrow from " << e.x1-0.5 << ',' << e.y1+0.5 << " to " << e.x2+0.5 << ',' << e.y2-0.5 << " nohead\n";
    }
    else {//verical line
      s << "set arrow from " << e.x1+0.5 << ',' << e.y1+0.5 << " to " << e.x2 - 0.5 << ',' << e.y2 - 0.5 << " nohead\n";
    }
  }
  //  s << "set arrow from 1,1 to 5,8 nohead\n";
  s << "plot -2\n";
  
}

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cout << "please provide the dimensions\n";
    return -1;
  }
  int X = std::stoi(argv[1]);
  int Y = std::stoi(argv[2]);
  initialize();
  std::string filename = std::to_string(X) + "x" + std::to_string(Y) + ".svg";
  std::fstream s(filename,s.out);
  if (!s.is_open()) {
    std::cout << filename << " could not be created\n";
    return -1;
  }
  std::vector<Edge> edges = make_labyrinth(X,Y);
  edges_to_svg(edges,s,X,Y);
  s.close();
}
