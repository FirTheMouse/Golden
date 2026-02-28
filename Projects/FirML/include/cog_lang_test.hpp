#pragma once

#define CRUMB_ROWS 1
#define CRUMB_COLS 4

#include<util/cog.hpp>
#include<gui/twig.hpp>

list<g_ptr<Crumb>> gather_states_from_context(const std::string& text, int pos) {
    list<g_ptr<Crumb>> states;
    g_ptr<Crumb> crumb = clone(ZERO);
    for(int i = 0; i < CRUMB_COLS; i++) {
        int p = pos - (CRUMB_COLS - 1) + i;
        crumb->mat[0][i] = (p >= 0) ? (float)text[p] / 127.0f : 0.0f;
    }
    return {crumb};
}

void run_language_test(g_ptr<Scene> scene) {
    auto source_code = make<Font>(root()+"/Engine/assets/fonts/source_code_black.ttf",100);

    g_ptr<Cog> cog = make<Cog>(scene);
    cog->span = make<Span>();

    float train_match = 0.98f;
    float train_lr = 0.1f;
    int train_con_threshold = 50;

    float sleep_match = 0.9966f;
    float sleep_lr = 0.1f;

    std::string training_text = 
    "the quick brown fox jumps over the lazy dog. "
    "a quick brown dog jumps over the lazy fox. "
    "the lazy dog sleeps under the warm sun. "
    "the quick fox runs through the green forest. "
    "a brown fox hunts in the quiet night. "
    "the dog and the fox are good friends. "
    "the quick brown fox jumps over the lazy dog. "
    "the lazy dog rests by the old tree. "
    "a quick fox sees a lazy dog sleeping. "
    "the brown dog plays with the quick fox. ";

    for(int n=0;n<1;n++) {
        if(n==0) {
            train_match = 0.50f;
            train_lr = -1.0f;
            train_con_threshold = 5;
        
            sleep_match = 0.9999f;
            sleep_lr = -1.0f;
        }
        else if(n==1) {
            sleep_match = 0.99999f;
        } else if(n==2) {
            sleep_match = 0.999999f;
        }

        cog->newline("training"); //Start TRN

        for(int i = 1; i < training_text.length(); i++) {
            //print("CHAR: ",i," RECENT_EPISODES: ",cog->recent_episodes.length()," CONSOLIDATED: ",cog->consolidated_episodes.length());
            g_ptr<Episode> ep = cog->form_episode(
                gather_states_from_context(training_text, i-1),
                gather_states_from_context(training_text, i),
                (int)training_text[i-1], i);
            if(cog->consolidate_episode(ep, cog->recent_episodes, ALL, train_match, train_lr) > train_con_threshold) {
                cog->recent_episodes.erase(ep);
                cog->consolidate_episode(ep, cog->consolidated_episodes, ALL, sleep_match, sleep_lr);
                // cog->log("Consolidated epiosde: ",ep->to_string());
            } else if(cog->recent_episodes.length()>1000) {
                for(int e=cog->recent_episodes.length()-1;e>=0;e--) {
                    g_ptr<Episode> to_consolidate = cog->recent_episodes[e];
                    if(to_consolidate->hits<=10) {
                        cog->recent_episodes.removeAt(e);
                    }
                }
            }
        }
        print("Run ",n+1," time: ",ftime(cog->endline())); //End TRN
        int pre_sleep_len = cog->consolidated_episodes.length();
        map<int,int> id_counts;
        for(int e=cog->consolidated_episodes.length()-1;e>=0;e--) {
            g_ptr<Episode> to_consolidate = cog->consolidated_episodes[e];
            id_counts.getOrPut(to_consolidate->action_id,0)+=1;
            cog->consolidated_episodes.removeAt(e);
            cog->consolidate_episode(to_consolidate,cog->consolidated_episodes, ALL, sleep_match, sleep_lr);
        }
        cog->span = make<Span>();

        std::string prompt = "the quick";
        std::string running = prompt;
        for(int i = prompt.length()-1; i < prompt.length() + 20; i++) {
            list<g_ptr<Crumb>> states = gather_states_from_context(running, i);
            float best_score = -1.0f;
            int predicted_char = -1;
            for(auto proto : cog->consolidated_episodes) {
                float score = cog->match_episode(proto, ALL, states, ALL);
                if(score > best_score) {
                    best_score = score;
                    predicted_char = proto->action_id;
                }
            }
            running += (char)predicted_char;
            print("Context: '", running.substr(std::max(0,(int)running.length()-CRUMB_COLS)), "' -> '", (char)predicted_char, "' [score: ", best_score, "]");
        }
        print("Prompt: '", prompt, "'");
        print("Continuation: '", running, "'");
        
        print("Training. match threshold: ",train_match," LR: ",train_lr," consolidate threshold: ",train_con_threshold);
        print("Sleep. match threshold: ",sleep_match," LR: ",sleep_lr);
        print("left over after sleep: ",cog->consolidated_episodes.length()," from: ",pre_sleep_len," recent: ",cog->recent_episodes.length());

        cog->consolidated_episodes.clear();
        cog->recent_episodes.clear();
    }

}


class babble_cog : public Cog {
    public:    
        std::set<char> seen_chars;

        list<g_ptr<Episode>> potential_reactions(list<g_ptr<Crumb>> crumbs, bool flip = false) override {
            list<g_ptr<Episode>> reactions;
            if(crumbs.empty()) return reactions;
        
            std::string current_context = context_from_crumbs(crumbs);
        
            for(char c : seen_chars) {
                std::string next_context = (current_context + c).substr(
                    std::max(0, (int)(current_context.length() + 1 - CRUMB_COLS))
                );
                list<g_ptr<Crumb>> next_states = gather_states_from_context(next_context, next_context.length()-1);
        
                float best_match = 0.0f;
                for(auto& ep : consolidated_episodes) {
                    float match = match_episode(ep, ALL, next_states, ALL);
                    if(match > best_match) best_match = match;
                }
        
                g_ptr<Episode> reaction = make<Episode>();
                reaction->states = next_states;
                reaction->action_id = (int)c;
                reaction->score = best_match;
                reactions << reaction;
            }
            return reactions; // no filtering, return all
        }
    
        // Reconstruct context string from crumb mat values
        std::string context_from_crumbs(list<g_ptr<Crumb>> crumbs) {
            if(crumbs.empty()) return "";
            g_ptr<Crumb> c = crumbs[0];
            std::string result = "";
            for(int i = 0; i < CRUMB_COLS; i++) {
                char ch = (char)(c->mat[0][i] * 127.0f);
                if(ch >= 32) result += ch;
            }
            return result;
        }
    
        float propagate(g_ptr<Episode> start, int& energy, 
            std::function<bool(g_ptr<Episode>, int&, int)> visit, 
            int depth = 0, float last_score = 0.0f) override {
                float best_match = 0.0f;
                for(auto& ep : consolidated_episodes) {
                    float match = match_episode(ep, ALL, start->states, ALL);
                    if(match > best_match) best_match = match;
                }
                
                if(energy <= 0 || depth >= 2) return best_match;
                
                // Look one step ahead
                list<g_ptr<Episode>> reactions = potential_reactions(start->states);
                if(reactions.empty()) return best_match;
                
                energy -= 1;
                float best_future = 0.0f;
                for(auto& r : reactions) {
                    float future = propagate(r, energy, visit, depth+1);
                    if(future > best_future) best_future = future;
                }
                
                return best_match * 0.7f + best_future * 0.3f;
        }

        char pick_next_char(const std::string& running) {
            list<g_ptr<Crumb>> current_states = gather_states_from_context(running, running.length()-1);
            
            list<g_ptr<Episode>> reactions = potential_reactions(current_states);
            
            print("  pick_next_char from context: '", 
                running.substr(std::max(0,(int)running.length()-CRUMB_COLS)), "'");
            print("  potential reactions: ", reactions.length());
            
            list<std::pair<g_ptr<Episode>, float>> candidates;
            int energy = 5;
        
            for(auto& reaction : reactions) {
                if(energy <= 0) break;
                float score = propagate(reaction, energy, [&](g_ptr<Episode> ep, int& remaining, int depth) {
                    return depth < 2;
                });
                print("    candidate: '", (char)reaction->action_id, "' score: ", score);
                candidates << std::make_pair(reaction, score);
            }
        
            // Winner takes all
            float best_score = 0.0f;
            g_ptr<Episode> winner = nullptr;
            for(auto& candidate : candidates) {
                if(candidate.second > best_score) {
                    best_score = candidate.second;
                    winner = candidate.first;
                }
            }
        
            char result = winner ? (char)winner->action_id : 'm';
            print("  winner: '", result, "' [score: ", best_score, "]");
            return result;
        }
    };


    void run_babble_test(g_ptr<Scene> scene) {
        g_ptr<babble_cog> cog = make<babble_cog>();
        cog->scene = scene;
        cog->span = make<Span>();
    
        float train_match = 0.5f;
        float train_lr = -1.0f;
        int train_con_threshold = 5;
        float sleep_match = 0.99f;
        float sleep_lr = -1.0f;
    
        std::string parent_speech = 
        "mama dada baba mama dada baba "
        "mama come mama go dada here ";
    
        auto train_on = [&](const std::string& text) {
            for(char c : text) cog->seen_chars.insert(c);
            for(int i = 1; i < text.length(); i++) {
                g_ptr<Episode> ep = cog->form_episode(
                    gather_states_from_context(text, i-1),
                    gather_states_from_context(text, i),
                    (int)text[i-1], i);
                if(cog->consolidate_episode(ep, cog->recent_episodes, ALL, train_match, train_lr) > train_con_threshold) {
                    cog->recent_episodes.erase(ep);
                    cog->consolidate_episode(ep, cog->consolidated_episodes, ALL, sleep_match, sleep_lr);
                }
            }
        };
    
        std::string running = "mamama";
    
        for(int age = 0; age < 20; age++) {
            // Babble to itself using propagate-based selection
            std::string self_babble = running.substr(std::max(0,(int)running.length()-CRUMB_COLS));
            for(int i = 0; i < 10; i++) self_babble += cog->pick_next_char(self_babble);
            if(age % 3 == 0) { train_on(parent_speech); running = parent_speech.substr(0,6); }
            for(int i = 0; i < 20; i++) self_babble += cog->pick_next_char(self_babble);
    
            print("Age ",age,": '",self_babble,"'");
            print("  Consolidated: ",cog->consolidated_episodes.length()," Recent: ",cog->recent_episodes.length());
        }
    }