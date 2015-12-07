# JsExec #

What: A JavaScript playground written in C++.

How: JsWrapper handles interactions with the Chakra JavaScript engine. The wrapper exports methods to JavaScript to call back into C++ code to modify the enviornment.

Why: Fun

Example:
![Example Image](http://i.imgur.com/J6mzz6c.png)
```javascript
for(var x=-360; x<360; x++)
{
  set_rotation(x/2,x,-x);
  set_color((0xFB2F00 + x).toString(16));
  sleep(10);
}
set_rotation(1,1,1)
```
