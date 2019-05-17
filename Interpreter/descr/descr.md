# descr

### contents

* [related file](#related-file)
* [memory layout](#memory-layout)


#### related file
* cpython/Objects/descrobject.c
* cpython/Include/descrobject.h

#### memory layout

[descriptor protocol in python](https://docs.python.org/3/howto/descriptor.html)

    from datetime import datetime
    dt = datetime.date
    print(type(dt))

