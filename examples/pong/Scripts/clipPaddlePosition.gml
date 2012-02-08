// make sure that the paddle is on the screen
if(bbox_top < 0) {
    y += -bbox_top;
} else if(bbox_bottom>room_height) {
    y -= bbox_bottom-room_height;
}
