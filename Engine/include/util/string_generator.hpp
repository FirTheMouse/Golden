#include<util/util.hpp>

//This was programed months before I even started on GDSL
namespace name {

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
                else toReturn = toReturn+meta[i][1]+s.get<str>(meta[i][1])+(i<meta.length()-1?"\n":"");
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

    const std::string STANDARD = "|, ,Ja|Be|Ma|Cer|Le,ck|de|ly|th|ch|un|el";
    const std::string AVAL_WEST_TAMOR = "|, ," "Bu|Ahm|He|Ol|Mo|In|Bir|Ba|Tu," "|||||||||||||||||||ha|ck|a|ch," "el|ba|ak|ael|he|med";

    std::string randname(const std::string& line = STANDARD) {
        return randsgen({line});
    }
}