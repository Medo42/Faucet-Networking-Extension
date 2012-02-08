// First, if we're the host we should notify the client so that it will run this code too.
if(instance_exists(GameServer)) {
    write_ubyte(GameServer.opponentSocket, 2);
    write_ubyte(GameServer.opponentSocket, PLAYER_SCORE);
    write_ubyte(GameServer.opponentSocket, argument0);
}

// Then, we actually perform the score event.
sound_play(ScoreSnd);

if(argument0 == 1) {
    ScorePanel.player1score += 1;
    Ball.hspeed = 5;
} else {
    ScorePanel.player2score += 1;
    Ball.hspeed = -5;
}

Ball.x = room_width/2;
Ball.y = room_height/2;
Ball.vspeed = random(10)-5;
