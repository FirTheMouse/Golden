#include<util/util.hpp>

namespace sgen {

    struct namebase {
        namebase() {}
        explicit namebase(const list<list<std::string>>& _opts) : opts(_opts) {}
        explicit namebase(const std::string& seed) {
            list<std::string> lines = split_str(seed,',');
            for(const auto& l : lines) {
                opts << split_str(l,'|');
            }
        }
        list<list<std::string>> opts;
    };

    const namebase STANDARD("Ja|Be|Ma|Cer|Le,ck|de|ly|th|ch|un|el");
    const namebase RANDOM(
        "A|a|B|b|C|c|D|d|E|e|F|f|G|g|H|h|I|i|J|j|K|k|L|l|M|m|N|n|O|o|P|p|Q|q|R|r|S|s|T|t|U|u|V|v|W|w|Y|y|X|x|Z|z,"
        "A|a|B|b|C|c|D|d|E|e|F|f|G|g|H|h|I|i|J|j|K|k|L|l|M|m|N|n|O|o|P|p|Q|q|R|r|S|s|T|t|U|u|V|v|W|w|Y|y|X|x|Z|z,"
        "A|a|B|b|C|c|D|d|E|e|F|f|G|g|H|h|I|i|J|j|K|k|L|l|M|m|N|n|O|o|P|p|Q|q|R|r|S|s|T|t|U|u|V|v|W|w|Y|y|X|x|Z|z,"
        "A|a|B|b|C|c|D|d|E|e|F|f|G|g|H|h|I|i|J|j|K|k|L|l|M|m|N|n|O|o|P|p|Q|q|R|r|S|s|T|t|U|u|V|v|W|w|Y|y|X|x|Z|z");
    const namebase AVAL_WEST_TAMOR_FIRST(
        "Bu|Ahm|He|Ol|Mo|In|Bir|Ba|Tu," 
        "|||||||||||||||||||ha|ck|a|ch," 
        "el|ba|ak|ael|he|med");

    const namebase AVAL_CENTRAL_FIRST_MALE(
        "Al|Ed|Da|Ro|Wil|Tho|Hen|Mar|Reg|Cla|Luc|Aug,"  
        "||||||||||||an|ar|er|or|ald|ric|vid|lan|den|bert|tor|mon,"
        "us|d|n|rt|mer|son|ard|ton|las|ius|mond|iel");

    const namebase AVAL_WESTERN_FIRST_MALE(
        "Jo|Al|Con|Se|Sok|Va|Wel|Eg," 
        "|||||||||rgo|ra|ell|ber,"
        "der|us|ard|rk|on|th|n|l|vid");

    const namebase AVAL_CENTRAL_FIRST_FEMALE(
        "My|Al|Se|Ma|Eg|Cha|Sha|Tha," 
        "|||||ri|ex|il,"
        "|||na|der|ra|us|da|na|et");
    const namebase AVAL_CENTRAL_LAST(
        "Copper|Silver|Iron|Wood|High|Low|Swift|Old|New|Red|White|Black|Green|Blue|Yellow," 
        "paw|tail|fang|talon|wing|feather|river|hill|heart|claw|hall");

    std::string randsgen(const namebase& g) {
        std::string result;
        for(const auto& s : g.opts) 
            result.append(s.rand());
        return result;
    }

    std::string randsgen(const std::string& line) {
        list<std::string> lines = split_str(line,',');
        std::string result;
        for(const auto& l : lines) {
            list<std::string> sub = split_str(l,'|');
            std::string app = sub.rand();
            result.append(app);
        }
        return result;
    }


namespace OLD {
    using str = std::string;

    list<str> newList(const str& s,char delimiter = ',')
    {
        list<str> toReturn;
        int last = 0;
        for(int i=0;i<s.length();i++)
        {
            if(s.at(i)==delimiter) {
                toReturn << s.substr(last,i-last);
                last = i+1;
            }
        }
        if(last<s.length())
        {
            toReturn << s.substr(last,s.length()-last);
        }
        return toReturn;
    }

    struct S_Result
    {
    S_Result() {}
    str cmd = "NONE";
    list<str> lst;
    str cbk = "NONE";
    };

    S_Result subParse(const str& s)
    {
        S_Result toReturn;
        auto nl = newList(s,'|');
        if(nl.length()<=1) {toReturn.lst = nl; return toReturn;}
        str x = nl[1];
        toReturn.cmd = x;
        toReturn.cbk = nl[0];
        nl.removeAt(0); nl.removeAt(0);
        toReturn.lst = nl;
        return toReturn;
    }

    bool contains(const str& s,const str& l)
    {
        int p = 0;
        for(int i=0;i<s.length();i++)
        {
            if(p==l.length()) return true;
            if(s.at(i)==l.at(p)) {
                p++;
            }
            else p=0;
        }
        return false;
    }

    struct gen {
    gen() {}
    str s;
    list<Data> dl;
    };

    gen generate(list<str> text,int times = 1)
    {
        list<list<str>> meta;
        for(int i=0;i<text.length();i++) meta << newList(text[i]);


        list<Data> finished;
        for(int i=0;i<times;i++)
        {
            Data s;
            for(auto l : meta) {
                if(l[0]!="X") {
                    char delmiter = l[0].at(0);
                    if(delmiter == '[')
                    {
                        auto sr = subParse(l[0]);
                        if(sr.cmd=="RANDF")
                        {
                            s.set<str>(l[1],std::to_string(randf(
                                std::stof(sr.lst[0]),
                                std::stof(sr.lst[1]))));
                        }
                        else if(sr.cmd=="RANDI")
                        {
                            s.set<str>(l[1],std::to_string(randi(
                                std::stof(sr.lst[0]),
                                std::stof(sr.lst[1]))));
                        }
                        else
                        {
                            s.set<str>(l[1],"INVALID OPPERATOR "+sr.cmd);
                        }
                    }
                    else {
                    str end = "";
                    for(int c=2;c<l.length();c++)
                    {
                        auto nl = newList(l[c],delmiter);
                        end = end+nl[randi(0,nl.length()-1)];
                    }
                    s.set<str>(l[1],end);
                    }
                }
                else { 
                    list<str> cl;
                    for(int j=2;j<l.length();j++) {
                        if(l[j].at(0)=='[')
                        {
                            list<str> nsl = newList(l[j],']');
                            int isValid = 0;
                            for(int m=0;m<nsl.length();m++)
                            {
                                auto sr = subParse(nsl[m]);
                                if(sr.cmd=="ANY") {
                                if(s.has(sr.lst[0])) {
                                        for(int c=1;c<sr.lst.length();c++) {
                                            if(sr.lst[c] == s.get<str>(sr.lst[0])) {
                                                isValid++;
                                                break;
                                            }
                                        }
                                    }
                                }
                                else if(sr.cmd=="REF") {
                                if(sr.lst.length()<=0)
                                {
                                    print(nsl[m]);
                                }
                                else if(s.has(sr.lst[0])) {
                                    str cbka = "";
                                    auto cbkal = newList(sr.cbk,'[');
                                    if(cbkal.length()>0) cbka = cbkal[0];
                                    if(cl.length()==0) cl <<cbka+s.get<str>(sr.lst[0]);
                                    else cl[0] = cl[0]+cbka+s.get<str>(sr.lst[0]);
                                }
                                }
                            }
                            if(isValid>=nsl.length()-1)
                            {
                                auto nnsl = newList(nsl[nsl.length()-1],'|');
                                cl << nnsl[randi(0,nnsl.length()-1)];
                            }
                            
                        }
                        else cl << l[j];
                    }
                    str toSet = "ERROR";
                    if(cl.length()>0) toSet = cl[randi(0,cl.length()-1)];
                    s.set<str>(l[1],toSet);}
                }
            finished << s;
        }
        str toReturn = "";
        finished([&,meta](Data& s){
            for(int i=0;i<meta.length();i++) {
                if(meta[i][1].at(0)=='!') continue;
                else toReturn = toReturn+(meta[i][1]==" "?"":meta[i][1])+s.get<str>(meta[i][1])+(i<meta.length()-1?"\n":"");
            }
            if(times>1) toReturn = toReturn+"\n----------------\n";
        });
        gen g;
        g.dl = finished;
        g.s = toReturn;
        return g;
    }

    std::string randsgen(list<std::string> lines,int times = 1) {
        gen done = generate(lines,times);
        return done.s;
    }
}


}