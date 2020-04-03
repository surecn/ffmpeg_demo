#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#include <QImage>



class ImageLoader
{
public:
    ImageLoader();
    virtual void load(QImage img);
};

#endif // IMAGELOADER_H
