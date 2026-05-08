void IRAM_ATTR readEncoder() {
  unsigned long now = millis();
  
  // 1. Minimum time between "clicks" (adjust 30-50 if still jumpy)
  if (now - lastTurnTime > 35) { 
    
    // 2. Read the pins immediately
    int L = digitalRead(ENC_L);
    int R = digitalRead(ENC_R);

    // 3. Quadrature logic
    if (L == R) {
      if (targetTemp < 30.0) targetTemp += 0.5; 
    } else {
      if (targetTemp > 5.0) targetTemp -= 0.5;  
    }
    
    lastTurnTime = now;
    needsDisplayUpdate = true;
  }
}
