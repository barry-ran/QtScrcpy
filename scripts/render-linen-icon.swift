import AppKit
let root = URL(fileURLWithPath: CommandLine.arguments[1])
func rounded(_ r: NSRect, _ radius: CGFloat, _ fill: NSColor, _ stroke: NSColor? = nil, _ width: CGFloat = 1) {
 let p = NSBezierPath(roundedRect:r,xRadius:radius,yRadius:radius);fill.setFill();p.fill();if let c=stroke {c.setStroke();p.lineWidth=width;p.stroke()}
}
func color(_ r:CGFloat,_ g:CGFloat,_ b:CGFloat)->NSColor {NSColor(srgbRed:r/255,green:g/255,blue:b/255,alpha:1)}
func render(_ size:Int, _ path:URL) throws {
 let image=NSImage(size:NSSize(width:size,height:size));image.lockFocus()
 let transform=NSAffineTransform();transform.scale(by:CGFloat(size)/1024);transform.concat()
 let linen=color(239,238,231), paper=color(251,251,246), sage=color(106,132,110), ink=color(66,86,70)
 rounded(NSRect(x:48,y:48,width:928,height:928),210,linen)
 rounded(NSRect(x:185,y:330,width:596,height:420),44,paper,sage,28)
 rounded(NSRect(x:221,y:368,width:524,height:344),17,color(225,231,219))
 let stand=NSBezierPath();stand.move(to:NSPoint(x:483,y:330));stand.line(to:NSPoint(x:483,y:242));stand.move(to:NSPoint(x:384,y:240));stand.line(to:NSPoint(x:580,y:240));stand.lineWidth=28;stand.lineCapStyle = .round;sage.setStroke();stand.stroke()
 rounded(NSRect(x:635,y:211,width:205,height:354),37,paper,ink,24)
 rounded(NSRect(x:659,y:252,width:157,height:273),15,color(209,221,202))
 let line=NSBezierPath();line.move(to:NSPoint(x:711,y:232));line.line(to:NSPoint(x:764,y:232));line.lineWidth=9;line.lineCapStyle = .round;ink.setStroke();line.stroke()
 image.unlockFocus();let rep=NSBitmapImageRep(data:image.tiffRepresentation!)!;try rep.representation(using:.png,properties:[:])!.write(to:path)
}
let dir=root.appendingPathComponent("local-icon.iconset");try FileManager.default.createDirectory(at:dir,withIntermediateDirectories:true)
for size in [16,32,128,256,512] {try render(size,dir.appendingPathComponent("icon_\(size)x\(size).png"));try render(size*2,dir.appendingPathComponent("icon_\(size)x\(size)@2x.png"))}
try render(1024,root.appendingPathComponent("local-icon-master.png"))
try render(256,root.appendingPathComponent("image/tray/logo.png"))
