//
// Created by DELL on 2023/4/27.
//

#ifndef HBBPLAYER_PLAYSTATUS_H
#define HBBPLAYER_PLAYSTATUS_H


class Playstatus {
public:
    bool exit;
    bool seek = false;
    bool pause = false;
    //    正在努力加载
    bool load = true;

public:
    Playstatus();
    ~Playstatus();
};


#endif //HBBPLAYER_PLAYSTATUS_H
