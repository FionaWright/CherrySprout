# New RMSE Tester Design

## Previous

Had two slots for textures, could switch between them

Golden always loaded into slot A

## New

Keep two slots, don't need any more

You can load golden into either slot

Saving/Loading golden explicit 

Use state machine for handling convergence test. Use GUI to set max parameters. Use selected slot for loading each frame