# JsExec #

What: A Windows Universal App with an embeded JavaScript playground.

Why: Fun

Example:
![Example Image](http://i.imgur.com/h2bMAYB.png)
```javascript
for(var x=-360; x<360; x++)
{
  if (x > 0)
    set_rotation(x/2,-x,x);
  else
    set_rotation(x/2,x,-x);

  set_color((0xFB2F00 + x).toString(16));

  sleep(10);
}
```
