Splash
======
<a class="donate" href="https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=curlymoo1%40gmail%2ecom&lc=US&item_name=curlymoo&no_note=0&currency_code=USD&bn=PP%2dDonationsBF%3abtn_donate_SM%2egif%3aNonHostedGuest" target="_blank">
<img alt="PayPal" title="PayPal" border="0" src="https://www.paypalobjects.com/en_US/i/btn/btn_donate_SM.gif" style="max-width:100%;"></a>
<a href="https://flattr.com/submit/auto?user_id=pilight&url=http%3A%2F%2Fwww.pilight.org" target="_blank"><img src="http://api.flattr.com/button/flattr-badge-large.png" alt="Flattr this" title="Flattr this" border="0"></a>

Template options:

As basic splash template with loader bar looks like this:
```
[{
	"name": "progress",
	"type" : "shape",
	"shape": "rectangle",
	"x": 265,
	"y": 175,
	"width": 272,
	"height": 149,
	"filled": 1,
	"z-index": 1,
	"color": [ 21, 49, 60 ]
}, {
	"name": "progress",
	"type" : "shape",
	"shape": "rectangle",
	"x": 235,
	"y": 146,
	"width": 330,
	"height": 207,
	"border": 26,
	"z-index": 1,
	"color": [ 249, 149, 29 ]
}, {
	"name": "logo",
	"type": "image",
	"y": -195,
	"z-index": 2,
	"image": "images/splash/new/logo.png"
}, {
	"name": "text",
	"type": "text",
	"x": 204,
	"y": 148,
	"size": 28,
	"align": "right",
	"decoration": "none",
	"spacing": 2,
	"font": "fonts/splash/Comfortaa-Light.ttf",
	"text": "loading...",
	"z-index": 2,
	"color": [ 150, 150, 155 ]
}, {
	"name": "progress",
	"type": "progress",
	"y": 110,
	"percentage": 0,
	"z-index": 2,
	"image": "images/splash/new/progressbar.png",
	"inactive": [ "images/splash/new/empty.png", "images/splash/new/empty.png", "images/splash/new/empty.png" ],
	"active": [ "images/splash/new/full.png", "images/splash/new/full.png", "images/splash/new/full.png" ]
}, {
	"name": "progress",
	"type" : "shape",
	"shape": "rectangle",
	"x": 192,
	"y": 402,
	"width": 415,
	"height": 33,
	"border": 3,
	"z-index": 2,
	"color": [ 150, 150, 155 ]
}]
```

A template can draw elements of different type and each type can have different options:

**shape**

rectangle

- name
- x
- y
- width
- height
- z-index
- color (array with r,g,b)
- filled
- fullscreen
- border

line

- name
- x1
- y1
- x2
- y2
- z-index
- thickness
- color (array with r,g,b)

circle

- name
- x
- y
- radius
- border
- z-index
- fullscreen
- filled
- color (array with r,g,b)

**image**

- name
- x
- y
- z-index
- image (both png and jpg are supported)

**text**

- name
- x
- y
- size
- align (left, center, right)
- decoration (none, italic)
- spacing
- font (ttf)
- text
- z-index
- color (array with r,g,b)

**progress**

- x
- y
- percentage
- z-index
- image (both png and jpg are supported)
- inactive (array with a start, middle, and end image)
- active (array with a start, middle, and end image)