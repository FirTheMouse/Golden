#include<core/helper.hpp>
#include<core/thread.hpp>
#include<util/logger.hpp>


class c_node : public Object {
public:
    c_node(char _c) : c(_c) {}
    char c;
    int freq_score = 0;
    int option_score = 0;
    map<char,g_ptr<c_node>> nodes;
    list<std::string> words;

    g_ptr<c_node> next(char ch) {
        return nodes.getOrPut(ch,make<c_node>(ch));
    }
};

g_ptr<c_node> root;
std::string abc = "abcdefghijklmnopqrstuvwxyz";

void gather_words(g_ptr<c_node> node,list<std::string>& result) {
    for(auto e : node->nodes.entrySet()) {
        result << e.value->words;
        gather_words(e.value,result);
    }
}

int count_words(g_ptr<c_node> node) {
    int result = node->words.length();
    for(auto e : node->nodes.entrySet()) {
        result += count_words(e.value);
    }
    return result;
}

list<std::string> words_from(const std::string& s) {
    list<std::string> result;
    g_ptr<c_node> node = root;
    for(auto c : s) {
        node = node->next(c);
        result << node->words;
    }
    gather_words(node,result);
    return result;
}

template<typename T>
void insert_by_score(list<T>& lst,const T& c,std::function<int(const T&)> func) {
    int score = func(c);
    int left = 0;
    int right = lst.size();
    
    while (left < right) {
        int mid = left + (right - left) / 2;
        int mid_score = func(lst[mid]);
        
        if (mid_score < score) {
            right = mid;
        } else {
            left = mid + 1;
        }
    }
    lst.insert(c,left);
}

list<char> ordered_list(g_ptr<c_node> node) {
    list<char> sub;
    for(char s : abc) {
        insert_by_score<char>(sub,s,[node](char c){
            return count_words(node->next(c));
        });
    }
    return sub;
}

int main() {
    using namespace helper;

    std::string MROOT = "../Projects/WordSquare/assets/models/";

    std::string file_text = readFile(MROOT+"wordle.csv");
    list<std::string> lines = split_str(file_text,'\n');
    list<std::string> words;
    for(int i=1;i<lines.length();i++) {
        auto sub = split_str(lines[i],',');
        words << sub[0];
    }

    root = make<c_node>(' ');
    for(auto w : words) {
        g_ptr<c_node> node = root;
        for(auto c : w)
            node = node->next(c);
        node->words << w;
    }

    char frequency[5][26];
    list<map<char,int>> row_freq;
    for(int i=0;i<5;i++) {
        map<char,int> freq;
        for(auto w : words)
            freq.getOrPut(w.at(i),0)+=1;
        row_freq.push(freq);
        list<char> sub;
        for(auto e : freq.entrySet()) {
            insert_by_score<char>(sub,e.key,[&freq](char c){
                return freq.get(c);
            });
        }
        for(int s=0;s<sub.length();s++) {
            frequency[i][s] = sub[s];
        }
    }

    // for(int c = 0;c<26;c++) {
    //     for(int r = 0;r<5;r++) {
    //         printnl(frequency[r][c]);
    //     }
    //     print("");
    // }

    g_ptr<c_node> snode = root;
    list<char> chars = ordered_list(root);
    char options[5][26];
    for(int c=0;c<26;c++) {
        snode = root;
        for(int r=0;r<5;r++) {
            if(r==0) {
                options[r][c] = chars[c];
            } else {
                list<char> sub = ordered_list(snode);
                options[r][c] = sub.first();
                for(int i=0;i<sub.length();i++) {
                    snode->next(sub[i])->option_score = (sub.length()-i);
                    snode->next(sub[i])->freq_score = row_freq.get(r).get(sub[i]);
                }
            }
            snode = snode->next(options[r][c]);
        }
    }

   

    // Row major
    // for(int r = 0;r<5;r++) {
    //     for(int c = 0;c<26;c++) {
    //         printnl(options[r][c]," ");
    //     }
    //     print("");
    // }
    //Column major
    for(int c = 0;c<26;c++) {
        for(int r = 0;r<5;r++) {
            printnl(options[r][c]);
        }
        print("");
    }

    // snode = root;
    // std::string test = "soare";
    // for(int r = 0;r<5;r++) {
    //    snode = snode->next(test.at(r));
    //    print(test.at(r),": ",snode->freq_score," ",snode->option_score," | ");
    // }

    // log::rig rig;
    // rig.add_process("pri",[](int i){
    //     words_from("pri");
    // });
    // rig.add_process("pra",[](int i){
    //     words_from("pra");
    // });
    // rig.add_comparison("pri","pra");
    // rig.run(1000);


    // std::string start = words[randi(0,words.length()-1)];
    // char square[5][5];
    // for(int r = 0;r<5;r++) {
    //     for(int c = 0;c<5;c++) {
    //         square[r][c] = ' ';
    //     }
    // }

    // for(int i=0;i<start.length();i++) {
    //     square[0][i] = start.at(i);
    // }

    // for(int r = 0;r<5;r++) {
    //     for(int c = 0;c<5;c++) {
    //         printnl(square[r][c]," ");
    //     }
    //     print("");
    // }

    // int inc = 0;
    // for(auto s : words_from("pri")) {
    //     if(inc++>10) {
    //         inc = 0;
    //         print(s);
    //     }
    //     else {
    //         printnl(s,", ");
    //     }
    // }




    // Window window = Window(1280, 768, "WordSquare 0.1");
    // g_ptr<Scene> scene = make<Scene>(window,2);
    // Data d = make_config(scene,K);
    //load_gui(scene, "WordSquare", "wordsquaregui.fab");
    // auto source_code = make<Font>(EROOT+"fonts/source_code.ttf",50);

    //auto thread = make<Thread>();
    // thread->run([&](ScriptContext& ctx){
 
    // },2.0f);
    // thread->start();

    // start::run(window,d,[&]{
    //     //auto text = text::makeText("T",source_code,scene,vec2(randf(0,1600),randf(0,1600)),1.0f);
    // });

    return 0;
}