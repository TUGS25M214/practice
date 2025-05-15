#include <iostream>
// STL:Standart Templete Library
// Container formats: vector, array, map, deque

#include <vector>
// vector: 可変長配列、要素末尾への追加・削除が早い O(1)

//int pow2(int x){ return x*x; }

int main(){
    std::vector<int> data;
    
    for(int a = 0, i = 0; a < 20; ++a){

        //ラムダ式
        auto pow2 = [&a] (int a){ return a*a; }; //[]参照、()引数

        if(a % 2 == 0){
            data.push_back(pow2(a));
        }
        // std::out << "a = " << a << std::endl;
    }

    int i = 0;
    for(auto v : data){
        std::cout << "data[" << i << " = " << data[i] << std::endl;
    }

    return 0;
}