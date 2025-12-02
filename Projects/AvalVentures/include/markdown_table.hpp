#pragma once
#include<core/helper.hpp>

namespace Golden {

    class m_table : public Object {
        public:
        int width = 0;
        list<std::string> headers;
        list<std::string> rows;
        list<std::string> contents;

        // std::string columns[6] = {"Trivial","Minor","Normal","Major","Severe","Permanent"};
        // std::string regions[7] = {"**Head**","**Face / Neck**","**Chest / Belly**","**Back / Shoulders**","**Arms / Paws**","**Legs / Feet**","**Tail / Base**"};

        m_table() {}
        m_table(const std::string& table) {
            list<std::string> sections = split_str(table,'|');
            for (auto& s : sections) {
                while (!s.empty() && s.front() == ' ')
                    s.erase(s.begin());
                while (!s.empty() && s.back() == ' ')
                    s.pop_back();
            }
            int spacer_start = -1;
            for(int i = 0;i<sections.length();i++) {
                if(sections[i]=="-----") {
                    width++;
                    if(spacer_start==-1) 
                            spacer_start = i;
                } else if (spacer_start!=-1) {
                    for(int w = 0; w<width;w++) {
                            sections.removeAt(spacer_start);
                    }
                    break;
                }
            }
            for(int i = 0; i<sections.length();i++) {
                if(i<width) {
                    headers << sections[i];
                } else {
                    if(i%width==0) rows << sections[i];
                    contents << sections[i];
                }
            }
        }
        m_table(int _width) {
            width = _width;
        }

        std::string to_string() {
            std::string result = "";
            for(auto h : headers) result.append(h+"|");
            for(int i=0;i<contents.length();i++) {
                if(i%width==0) result.append("\n");
                result.append(contents[i]+"|");
            }
            return result;
        }

        std::string to_markdown() {
            std::string result = "";
            for(auto h : headers) result.append(" "+h+" |");
            result.append("\n");
            for(int w = 0;w<width;w++) result.append(" ------ |");
            for(int i=0;i<contents.length();i++) {
                if(i%width==0) result.append("\n");
                result.append(" "+contents[i]+" |");
            }
            return result;
        }

        std::string to_terminal() {
            std::string result = "\"";
            for(auto h : headers) result.append(" "+h+" |");
            result.append("\"\n\"");
            for(int w = 0;w<width;w++) result.append(" ------ |");
            for(int i=0;i<contents.length();i++) {
                if(i%width==0) result.append("\"\n\"");
                result.append(" "+contents[i]+" |");
            }
            result.append("\"");
            return result;
        }

        void print_out() {
            // print("headers: "); 
            // for(auto h : headers) {
            //      print("   "+h);
            // }
            // print("Contents: "); 
            for(int i=0;i<contents.length();i++) {
                if(i%width==0) 
                    print(contents[i]);
                else
                    print("   "+headers[i%width]+": "+contents[i]);
                // int at = contents[i].find('*');
                // if(at==std::string::npos) {
                //      print("  "+headers[i%width]+": "+contents[i]);
                // } else if(at<contents[i].length()-1) {
                //      int next = contents[i].find('*',at+1);

                //           if(contents[i].at(at+1)=='*') {
                //                //std::string head_section = contents[i].substr(at,contents[i].find('*',at+2)-(at-2));
                //                for(int i=0;i<3;i++) {
                //                     at = next;
                //                     next = s.find('*',at+1);
                //                }
                //           }
                // }
            }
        }
        
        list<std::string> column_contents(int column) {
            list<std::string> result;
            for(int i=0;i<contents.length();i++) { 
                if(i%width==column) result << contents[i];
            }
            return result;
        }

        list<std::string> row_contents(int row) {
            list<std::string> result;
            int on_row = -1;
            for(int i=0;i<contents.length();i++) { 
                if(i%width==0) on_row++;
                if(on_row==row) {
                    result << contents[i];
                } else if (on_row > row) break;
            }
            return result;
        }

        size_t find_and_infer(const std::string& section,const std::string& thing,int infer,int depth = 0) {
            int at = section.find(thing);
            if(at!=std::string::npos) {
                return at;
            } else {
                std::string s = section;
                std::string t = thing;
                switch(infer) {
                    case 1: 
                    {
                            std::transform(s.begin(), s.end(), s.begin(),[](unsigned char c){ return std::tolower(c);});
                            std::transform(t.begin(), t.end(), t.begin(),[](unsigned char c){ return std::tolower(c);});
                            return find_and_infer(s,t,2);
                    }
                    break;

                    case 2:
                    {
                            if(depth==0) return find_and_infer(s,split_str(t,'/')[0],2,1);
                            else if(depth==1) return find_and_infer(s,split_str(t,'(')[0],3,0);
                    }
                    break;

                    default: //Also 0 or No inference
                            return std::string::npos;
                }
            }
            return std::string::npos;
        }
        int column_from_string(const std::string& in_column,int infer = 1) {
            for(int i=0;i<width;i++) {
                if(find_and_infer(headers[i],in_column,infer)!=std::string::npos) {
                    return i;
                }
            }
            return -1;
        }
        int row_from_string(const std::string& in_row,int infer = 1) {
            for(int i=0;i<contents.length()/width;i++) {
                if(find_and_infer(contents[i*width],in_row,infer)!=std::string::npos) {
                    return i;
                }
            }
            return -1;
        }
        //Inference level controls how much auto-correct the procceser will add
        //0 = None at all, read as written
        //
        void process_command(const std::string& command,int infer = 1,bool debug = false) {
            list<std::string> sections = split_str(command,'|');
            for(auto s : sections) {
                list<std::string> parts = split_str(s,';');
                list<std::string> args;
                if(parts.length()>2) {
                    for(int i = 3 ;i<parts.length();i++) {
                            args << parts[i];
                    }
                }
                execute_command(parts[0],parts[1],parts[2],args,infer,debug);
            }
        }     
        void execute_command(const std::string& in_column, const std::string& in_row,const std::string& thing, 
                            list<std::string> args,int infer = 1,bool debug = false) {
            int column_num = column_from_string(in_column,infer);
            int row_num = -1;
            if(column_num == -1) {
                print("No such column found: ",in_column);
                return;
            }
            if(in_row == "all") {
                for(int r = 0;r<rows.length();r++) {
                    execute_command(column_num,r,thing,args,infer,debug);
                }
            } else {
                list<std::string> rows = split_str(in_row,':');
                if(rows.length()==0) print("Missing a row ");
                else if(rows.length()==1) {
                    row_num = row_from_string(rows[0],infer);
                } else {
                    for(auto i = 0;i<rows.length();i++) {
                            row_num = row_from_string(rows[i],infer);
                            if (row_num == -1) {
                                print("No such row found: ",rows[i]);
                                continue;
                            }
                            execute_command(column_num,row_num,thing,args,infer,debug);
                    }
                    return;
                }

                if (row_num == -1) {
                    print("No such row found: ",in_row);
                    return;
                }
                execute_command(column_num,row_num,thing,args,infer,debug);
            }
        }
        const std::string command_args[5] = {"ADD_IN","REMOVE_IN","MOVE_TO","INC","NEW_DESC"};
        bool is_command_arg(const std::string& arg) {
            for(int i = 0;i<4;i++) {
                if(arg==command_args[i]) return true;
            }
            return false;
        }
        void execute_command(int in_column, int in_row,const std::string& thing, list<std::string> args, int infer = 1,bool debug = false) { 
            int id = (in_row*width)+in_column;
            std::string s = contents[id];
            for(auto arg : args) {
                auto sub_arg = split_str(arg,':');
                std::string t = thing;
                if(sub_arg.length()==0) {
                    contents[id].append(thing);
                } else {
                    arg = sub_arg[0];
                    std::string loc = "";
                    if(sub_arg.length()>1) loc = sub_arg[1];

                    if(thing.find('-')!=std::string::npos&&arg!="ADD_IN") {
                            int astr = thing.find('*',2);
                            t = thing.substr(1,astr);
                    }

                    if(arg=="ADD_IN") {
                            int at = find_and_infer(s,loc,infer);
                            if(at==std::string::npos) {
                                print("Unable to find arg loc: ",loc," In: ",s);
                            } else
                                contents[id].insert(at+loc.length(),t);
                    }
                    else if (arg=="REMOVE_IN") {
                            int at = find_and_infer(s,t,infer);
                            if(at==std::string::npos) {
                                print("Unable to find t to remove: ",t," In: ",contents[in_row*width],", ",headers[in_column]);
                            } else {
                                int next = s.find('*',at+t.length());
                                if(debug) print("NEXT: ",next," AT: ",at,", REMOVING: ",contents[id].substr(at,(next-at)));
                                contents[id].erase(at,next-at);
                            }
                    } else if (arg=="MOVE_TO") {
                            int at = find_and_infer(s,t,infer);
                            std::string full_t = "";
                            if(at==std::string::npos) {
                                print("Unable to find t for move: ",t," In: ",contents[in_row*width],", ",headers[in_column]);
                            } else {
                                int next = s.find('*',at+t.length());
                                full_t = s.substr(at,next-at);
                            }
                            std::string new_cmd = "";
                            for(int a = 1;a<sub_arg.length();a++) {
                                if(a==3) {
                                    new_cmd.append(full_t+";"+"ADD_IN:");
                                }
                                new_cmd.append(sub_arg[a]);
                                if(a<sub_arg.length()-1) {
                                    new_cmd.append(";");
                                } 
                            }
                            execute_command(in_column,in_row,t,{"REMOVE_IN:"},infer,debug);
                            process_command(new_cmd,infer,debug);
                    } else if(arg=="INC") {
                            int at = find_and_infer(s,t,infer);
                            if(at==std::string::npos) {
                                print("Unable to find t for inc: ",t," In: ",contents[in_row*width],", ",headers[in_column]);
                            } else {
                                int ex = s.find('x',at)+1;
                                if(ex!=std::string::npos) {
                                    int num = ex+1;
                                    while(std::isdigit(s.at(num))) {
                                        num++;
                                    } 
                                    std::string ns = s.substr(ex,num-ex);
                                    contents[id].erase(ex,ns.length());
                                    int inc_num = std::stoi(ns);
                                    int inc_val = 1;
                                    if(loc!="") inc_val = std::stoi(loc);
                                    //if(thing==" *THING HERE* ") reg_inf[in_row]+=inc_val;
                                    contents[id].insert(ex,std::to_string(inc_num+inc_val));
                                } else {
                                    print("Failed to find ex for inc");
                                }
                            }
                    } else if(arg=="NEW_DESC") {
                            int at = find_and_infer(s,t,infer);
                            if(at==std::string::npos) {
                                print("Unable to find t for new_desc: ",t," In: ",contents[in_row*width],", ",headers[in_column]);
                            } else {
                                int ex = s.find('-',at)+1;
                                if(ex!=std::string::npos) {
                                    int next = s.find('*',ex);
                                    if(debug) print("NEXT: ",next," AT: ",ex,", REMOVING: ",contents[id].substr(ex,(next-ex)));
                                    contents[id].erase(ex,(next-ex));
                                    contents[id].insert(ex,loc);
                                }
                            }
                    }
                    else {
                            print("Unrecognized arg: ",arg);
                    }
                }
            }
        }

        int value_of(const std::string& in_column, const std::string& in_row,const std::string& thing,int infer = 1) {
            int column_num = column_from_string(in_column,infer);
            int row_num = row_from_string(in_row,infer);
            
            if(column_num == -1) {
                print("No such column found: ",in_column);
                return 0;
            }
            if (row_num == -1) {
                print("No such row found: ",in_row);
                return 0;
            }
            return value_of(column_num,row_num,thing,infer);
        }

        int value_of(int in_column, int in_row,const std::string& thing,int infer = 1) {
            int id = (in_row*width)+in_column;
            std::string s = contents[id];
            std::string t = thing;
            int at = find_and_infer(s,t,infer);
            if(at==std::string::npos) {
                print("Unable to find t for value of: ",t," In: ",contents[in_row*width],", ",headers[in_column]);
            } else {
                int ex = s.find('x',at)+1;
                if(ex!=std::string::npos) {
                    int num = ex+1;
                    while(std::isdigit(s.at(num))) {
                            num++;
                    } 
                    std::string ns = s.substr(ex,num-ex);
                    int inc_num = std::stoi(ns);
                    return inc_num;
                } else {
                    print("Failed to find ex for value of");
                }
            }
            return 0;
        }


        list<std::string> headers_in_section(const std::string& in_column, const std::string& in_row, std::string loc = "",bool return_headers = true) {
            int infer = 1;
            int column_num = column_from_string(in_column,infer);
            int row_num = row_from_string(in_row,infer);
            
            if(column_num == -1) {
                print("No such column found: ",in_column);
                return list<std::string>{};
            }
            if (row_num == -1) {
                print("No such row found: ",in_row);
                return list<std::string>{};
            }
            return headers_in_section(column_num,row_num,loc,return_headers);
        }

        list<std::string> headers_in_section(int in_column, int in_row,std::string loc = "",bool return_headers = true) {
            int infer = 1;
            list<std::string> result;
            int id = (in_row*width)+in_column;
            std::string s = contents[id];
            int at = find_and_infer(s,"*",infer);
            if(at==std::string::npos) {
                print("No headers for section in: ",contents[in_row*width],", ",headers[in_column]);
            } else if(at<s.length()-1) {
                int next = s.find('*',at+1);
                int depth = 0;
                bool is_header = !return_headers;
                bool in_section = true;
                while(next!=std::string::npos&&depth<10000) {
                    next = s.find('*',at+1);
                    depth++;
                    if(s.at(at+1)=='*') {
                            //print("FOUND A SECTION HEADER: ",s.substr(at,s.find('*',at+2)-(at-2)));
                            if(loc!="") {
                                std::string head_section = s.substr(at,s.find('*',at+2)-(at-2));
                                if(find_and_infer(head_section,loc,infer)!=std::string::npos) {
                                    in_section = true;
                                } else {
                                    in_section = false;
                                }
                            } 
                            for(int i=0;i<3;i++) {
                                at = next;
                                next = s.find('*',at+1);
                            }
                    } else if(in_section) {
                            is_header = !is_header;
                            if(is_header) {
                                //print("FOUND A HEADER: ",s.substr(at,next-at+1));
                                result << s.substr(at,next-at+1);
                            }
                    }
                    at = next;
                }
            }
            if(!return_headers) {
                for(auto& r : result) {
                    r = trim_str(r,'*');
                    if(r.front()==' ') r = r.substr(0,r.length()-1);
                }
            }

            return result;
        }
    };

    void promote(g_ptr<m_table> table) {
        bool promote = true;

        list<std::string> sections = {"**Injuries**","**Filth**"};
        list<std::string> columns = table->headers;
        columns.removeAt(0); columns.removeAt(0); //Clearing the back parts
        list<std::string> regions = table->rows;
        for(int o = 0;o<1;o++)
        for(auto s : sections) {
            for(int r=0;r<7;r++) {
                for(int c=0;c<6;c++) { 
                    //print(regions[r],": ",columns[c]);
                    list<std::string> contents = table->headers_in_section(columns[c],regions[r],s,false);
                    list<std::string> headers = table->headers_in_section(columns[c],regions[r],s,true);
                    for(int i=0;i<headers.length();i++) {
                            std::string h = headers[i];
                            std::string d = contents[i];
                            // printnl(h,": "); print(table->value_of(columns[c],regions[r],h));
                            if(promote) {
                                float promote_f = table->value_of(columns[c],regions[r],h)/3;
                                int promote_i = std::floor(promote_f);
                                if(c<5&&promote_i>0) {
                                    if(table->headers_in_section(columns[c+1],regions[r],s).has(h)) {
                                        // if(h.substr(0,2)=="  ") h = h.substr(2,h.length()-1);
                                        table->execute_command(columns[c+1],regions[r],h,{"INC:"+std::to_string(promote_i)});
                                    } else {
                                        if(h.back() != ' ') h = " "+h;
                                        table->execute_command(columns[c+1],regions[r],h+" x"+std::to_string(promote_i)+" "+d.substr(d.find('-')),{"ADD_IN:"+s});
                                    }
                                    if(h.back() != ' ') h = h.substr(1,h.length()-1);
                                    table->execute_command(columns[c],regions[r],h,{"INC:"+std::to_string(promote_i*-3)});
                                }
                            }
                            if(table->value_of(columns[c],regions[r],h)<=0) {
                                table->execute_command(columns[c],regions[r],h,{"REMOVE_IN:"});
                            }
                    }
                }
            }
        }

        //Final cleanup
        for(auto& t : table->contents) {
            int next = t.find('*');
            while(next>0&&next<t.length()) {
                if(std::isalpha(t.at(next-1)&&t.at(next+1)!='*'&&t.at(next+1)!=' ')) {
                    t.insert(next+1," ");
                }
                next = t.find('*',next+1);
            }
        }
    }
    


    const std::string std_table =
        "Region | Gear | Trivial | Minor | Normal | Major | Severe | Permanent |"
        "----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- |"
        "**Head** | None | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth**  | **Injuries**  **Filth** | **Injuries**  **Filth** |"
        "**Face** | None | **Injuries**  **Filth**  | **Injuries**  **Filth**  | **Injuries**  **Filth**  | **Injuries**  **Filth**  | **Injuries**  **Filth** | **Injuries**  **Filth** |"
        "**Chest** | None | **Injuries** **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth**  | **Injuries**  **Filth** | **Injuries**  **Filth**  | **Injuries**  **Filth** |"
        "**Back** | None | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** |"
        "**Forelimbs** | None | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** |"
        "**Hindlimbs** | None | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth**  | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** |"
        "**Other** | None | **Injuries**  **Filth** | **Injuries**  **Filth**  | **Injuries**  **Filth**  | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** |";

}
