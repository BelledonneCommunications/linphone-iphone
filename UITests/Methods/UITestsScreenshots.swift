import SwiftUI
import XCTest

extension XCUIElement {
	private static var _representation = [String:UITestsScreenshots]()
	
	// hack to add variable in extensions
	var representation: UITestsScreenshots {
		get {
			let tmpAddress = String(format: "%p", unsafeBitCast(self, to: Int.self))
			if (XCUIElement._representation[tmpAddress] == nil) {
				XCUIElement._representation[tmpAddress] = UITestsScreenshots(element: self)
			}
			return XCUIElement._representation[tmpAddress]!
		}
	}
}

extension XCUIApplication {
	private static var _representationWithElement = UITestsAppRepresentation()
	
	var representationWithElements: UITestsAppRepresentation {
		get {
			return XCUIApplication._representationWithElement
		}
	}
}

class UITestsAppRepresentation : UITestsScreenshots {
	
	var mainView: XCUIElement
	var otherElement: XCUIElement?
	private let statusBar: XCUIElement
	private let tabBar: XCUIElement
	private let app = XCUIApplication()
	
	private(set) var allVariations = [[String](),[String](),[String](),[String]()]
	
	private static var backup: [String:(XCUIElement,XCUIElement?,[[String]])] = [:]
	
	private var elementsDescription = ""
	override var description: String {
		get {
			let description = super.description + " (definition = " + elementsDescription + ")"
			elementsDescription = ""
			return description
		}
	}
	
	init() {
		self.mainView = app.dialerView
		self.statusBar = app.statusBar
		self.tabBar = app.tabBar
		super.init(element: app)
	}
	
	func withElementVariations(mainView: [String], statusBar: [String], tabBar: [String], other: [String] = []) -> UITestsAppRepresentation {
		allVariations = [mainView, statusBar, tabBar, other]
		return self
	}
	
	func updateElementVariations(mainView: [String], statusBar: [String], tabBar: [String], other: [String] = []) -> UITestsAppRepresentation {
		allVariations[0] += mainView
		allVariations[1] += statusBar
		allVariations[2] += tabBar
		allVariations[3] += other
		return self
	}
	
	override func convertForComparaison(screenshot: UIImage) -> UIImage {
		let elements = getElements()
		if (svgManager.rects["mask"] == nil) {_=svgManager.parse()}
		_=elements.map{
			if ($0 != nil) {
				if ($0!.representation.svgManager.rects["mask"] == nil) {_=$0!.representation.svgManager.parse()}
				svgManager.rects["mask"]! += $0!.representation.svgManager.rects["mask"]!
			}
		}
		return super.convertForComparaison(screenshot: screenshot)
	}
	
	override func getReference() -> UIImage? {
		UIGraphicsBeginImageContextWithOptions(UITestsScreenshots.screenSize, false, 1)
		guard (super.getReference()?.draw(at: CGPoint(x: 0, y: 0)) != nil) else {return nil}
		let elements = getElements()
		for i in 0..<elements.count {
            if (elements[i] != nil) {
                elements[i]!.representation.withVariations(named: allVariations[i]).getReference()?.draw(at: CGPoint(x: 0, y: 0))
                elementsDescription += elements[i]!.representation.description + " + "
            }
		}
		let reference = UIGraphicsGetImageFromCurrentImageContext()!
		UIGraphicsEndImageContext()
		return reference
	}
	
	func getElements() -> [XCUIElement?]{
		return [mainView,statusBar,tabBar,otherElement].map {($0 != nil && self.element.frame.contains($0!.frame)) ? $0! : nil}
	}

	func makeBackup(named: String) {
		UITestsAppRepresentation.backup[named] = (mainView,otherElement,allVariations)
	}
	
	func reloadBackup(named: String) -> UITestsAppRepresentation {
		if let backup = UITestsAppRepresentation.backup[named] {
			mainView = backup.0
			allVariations = backup.2
			otherElement = backup.1
			UITestsAppRepresentation.backup.removeValue(forKey: named)
		} else {
			XCTFail("unable to find an app representation backup named \"\(named)\"")
		}
		return self
	}
	
}

class UITestsScreenshots {
	
	static let screenshotDelay: TimeInterval = 0.5
	static let pixelTreshold: Int = 3
	static let colorTreshold: Int = 3
	static let screenSize: CGSize = {
		var size = XCUIApplication().frame.size
		let scaleFactor = 3.0
		size.width.scale(by: scaleFactor)
		size.height.scale(by: scaleFactor)
		return size
	}()
	static let defaultPath: String  = {
			let path = #filePath
			return String(path.prefix(path.distance(from: path.startIndex, to: path.range(of: "UITests")!.lowerBound) + "UITests/".count)) + "Screenshots/"
	}()
	internal var description: String {
		get {
            return viewName + (variations.isEmpty ? "" : "||\(variations.joined(separator: ","))")
		}
	}
	private(set) var debugHistory: String = ""
	
	internal let element: XCUIElement
	private var _svgManager: SVGManager?
	internal var svgManager: SVGManager {
		get{
			if (_svgManager == nil) {_svgManager = SVGManager(path: "\(UITestsScreenshots.defaultPath + viewName).svg")}
			return _svgManager!
		}
	}
	internal var _viewName: String?
	var viewName: String {
		get {
			if (_viewName == nil) {
				_viewName = element.identifier
				if (_viewName!.isEmpty) {_viewName = element.label}  //for elements wich don't have identifier
				debugHistory = "UITestsScreenshots : \(_viewName!) : "
			}
			return _viewName!
		}
	}
	private(set) var variations = [String]()
	
	init(element: XCUIElement) {
		self.element = element
	}
	
	// public functions
	
	func withVariations(named: [String]) -> UITestsScreenshots {
		variations = named
		debugHistory += " with varitions named \"\(named.joined(separator: "\", \""))\" -> "
		return self
	}
	
	func make(after time: TimeInterval = screenshotDelay) {
		XCTContext.runActivity(named: "Make \"\(viewName)\" reference screenshot") { context in
			debugHistory += "make reference -> "
			guard checkVariationNonDefinition(), referenceExist(expectedValue: false), var screenshot = takeScreenshot(after: time) else {return}
			saveImage(image: screenshot, path: getPath(name: viewName))
			svgManager.createFile(referenceName: viewName, referenceArea: getElementArea(), svgSize: UITestsScreenshots.screenSize)
			screenshot = UITestsScreenshots.imageInScreenAcrea(image: screenshot, area: getElementArea())
			let preview = UITestsScreenshots.createPreview(title: "Reference", image: screenshot)
			context.add(UITestsScreenshots.createAttachement(image: preview, name: description))
			debugHistory += "done."
		}
	}
	
	func reMake(after time: TimeInterval = screenshotDelay) {
		XCTContext.runActivity(named: "Remake \"\(viewName)\" reference screenshot") { context in
			debugHistory += "re make reference -> "
			guard referenceImagesExist(names: [viewName]+variations, expectedValue: true), var screenshot = takeScreenshot(after: time) else {return}
			_ = (variations.isEmpty ? [viewName] : variations).map{
				saveImage(image: screenshot, path: getPath(name: $0))
				_ = svgManager.updateImage(name: $0, area: getElementArea())
			}
			screenshot = UITestsScreenshots.imageInScreenAcrea(image: screenshot, area: getElementArea())
			let preview = UITestsScreenshots.createPreview(title: "Reference", image: screenshot)
			context.add(UITestsScreenshots.createAttachement(image: preview, name: description))
			debugHistory += "done."
			XCTFail("\"\(#function)\" is a temporary function, you can't succeed a test with it\nafter remaking a reference you have to use \"check()\" if you want to compare")
		}
	}
	
	func check(after time: TimeInterval = screenshotDelay) {
		XCTContext.runActivity(named: "Check \"\(viewName)\" screenshot with his reference") { context in
			debugHistory += "compare screenshot to reference -> "
			guard var screenshot = takeScreenshot(after: time), let reference = getReference() else {return}
			screenshot = convertForComparaison(screenshot: screenshot)
			guard let variances = UITestsScreenshots.getVarianceAreas(reference, screenshot) else {return}
			if (!variances.areas.isEmpty) {
				let errorMsg = "variances found with the reference view in \(variances.areas.count) areas."
				debugHistory += errorMsg
				XCTFail(errorMsg)
			} else {
				debugHistory += "done."
			}
			let preview = UITestsScreenshots.comparativePreview(reference: reference, screenshot: screenshot, difference: variances.image, areas: variances.areas)
			context.add(UITestsScreenshots.createAttachement(image: preview, name: description))
			
		}
	}
	
	func addNewVariation(named name: String, after time: TimeInterval = screenshotDelay) {
		XCTContext.runActivity(named: "Add \"\(viewName)\" reference screenshot new varation named \(name)") { context in
			debugHistory += "add variances to a new variation named \(name) -> "
			guard referenceImagesExist(names: [name], expectedValue: false), let screenshot = takeScreenshot(after: time), let reference = getReference() else {return}
			guard let variances = UITestsScreenshots.getVarianceAreas(reference, convertForComparaison(screenshot: screenshot)) else {return}
			guard !variances.areas.isEmpty else {
				XCTFail(debugHistory + "error! : no variances found with the reference view")
				return
			}
			saveImage(image: screenshot, path: getPath(name: name))
			_ = svgManager.addVariation(referenceName: viewName, name: name, area: getElementArea(), rects: variances.areas)
			let preview = UITestsScreenshots.variationPreview(reference: reference, areas: variances.areas)
			context.add(UITestsScreenshots.createAttachement(image: preview, name: description))
			debugHistory += "done."
		}
	}
	
	func addNewFilterVariation(named name: String, color: UIColor, areas: [CGRect]) {
		XCTContext.runActivity(named: "Add \"\(viewName)\" reference screenshot new filter varation named \(name)") { context in
			let size = UITestsScreenshots.screenSize
			UIGraphicsBeginImageContextWithOptions(size, false, 1)
			color.setFill()
			UIRectFillUsingBlendMode(CGRect(x: 0, y: 0, width: size.width, height: size.height), .normal)
			let image = UIGraphicsGetImageFromCurrentImageContext()!
			UIGraphicsEndImageContext()
			saveImage(image: image, path: getPath(name: name))
			_ = svgManager.addVariation(referenceName: viewName, name: name, area: getElementArea(), rects: areas)
			context.add(UITestsScreenshots.createAttachement(image: image, name: description))
			debugHistory += "done."
		}
	}
	
	func addToMask(after time: TimeInterval = screenshotDelay) {
		XCTContext.runActivity(named: "Add new areas to \"\(viewName)\" mask") { context in
			debugHistory += "add variances to mask -> "
			guard let screenshot = takeScreenshot(after: time), let reference = getReference() else {return}
			guard let variances = UITestsScreenshots.getVarianceAreas(reference, convertForComparaison(screenshot: screenshot)) else {return}
			guard !variances.areas.isEmpty else {
				XCTFail(debugHistory + "error! : no variances found with the reference view")
				return
			}
			_ = svgManager.addToMask(rects: variances.areas)
			let preview = UITestsScreenshots.variationPreview(reference: reference, areas: variances.areas)
			context.add(UITestsScreenshots.createAttachement(image: preview, name: description))
			debugHistory += "done."
		}
	}
	
	func isAnimated(timeInterval: TimeInterval) {
		XCTContext.runActivity(named: "Check \"\(viewName)\" animation") { context in
			debugHistory += "check if element is animated -> "
			if (takeScreenshot(after: UITestsScreenshots.screenshotDelay)?.pngData() == takeScreenshot(after: timeInterval)?.pngData()) {
				XCTFail("no animation detected for \"\(viewName)\"")
			}
			debugHistory += "done."
		}
	}
	
	func takeScreenshot(after time: TimeInterval) -> UIImage? {
		XCTContext.runActivity(named: "take screenshot") { context in
			debugHistory += "take screenshot -> "
			_=XCTWaiter.wait(for: [XCTestExpectation()], timeout: time)
			return UIImage(data: element.screenshot().pngRepresentation)
		}
	}
	
	func getReference() -> UIImage? {
		debugHistory += "get reference -> "
		guard svgManager.parse(withVariations: variations), referenceImagesExist(names: [viewName]+variations, expectedValue: true) == true else {return nil}
		let filePaths = [getPath(name: viewName)]+variations.map{getPath(name: $0)}
		let imagesName = [viewName] + variations.map{viewName+"_"+$0}
		var images = [UIImage]()
		for i in 0...imagesName.count-1 {
			let image = getImage(path: filePaths[i])!
			let area = svgManager.images[imagesName[i]]!.area
			var clip = [CGRect]()
			if (i>0) {clip = svgManager.rects[variations[i-1]]!}
			images.append(UITestsScreenshots.imageInScreenAcrea(image: image, area: area, clip: clip))
			
		}
		
		UIGraphicsBeginImageContextWithOptions(UITestsScreenshots.screenSize, false, 1)
		_ = images.map{$0.draw(at: CGPoint(x: 0,y: 0))}
		let reference = UIGraphicsGetImageFromCurrentImageContext()!
		UIGraphicsEndImageContext()
		return imageWithMask(image: reference, mask: svgManager.rects["mask"]!)
	}
	
	//create attachement to return when calling public functions
	
	static func createAttachement(image: UIImage, name: String, lifetime: XCTAttachment.Lifetime = .deleteOnSuccess) -> XCTAttachment {
		let attachment = XCTAttachment(image: image)
		attachment.lifetime = lifetime
		attachment.name = name
		return attachment
	}

	//image operations to prepare comparison
	
	static func imageInScreenAcrea(image: UIImage, area: CGRect, clip: [CGRect] = []) -> UIImage {
		UIGraphicsBeginImageContextWithOptions(UITestsScreenshots.screenSize, false, 1)
		if (!clip.isEmpty) {UIGraphicsGetCurrentContext()!.clip(to: clip)}
		image.draw(at: CGPoint(x: area.minX, y: area.minY))
		let screenImage = UIGraphicsGetImageFromCurrentImageContext()!
		UIGraphicsEndImageContext()
		return screenImage
	}
	
	static func setBackground(image: UIImage, color: UIColor) -> UIImage {
		let size = UITestsScreenshots.screenSize
		UIGraphicsBeginImageContextWithOptions(size, false, 1)
		color.setFill()
		UIRectFill(CGRect(x: 0, y: 0, width: size.width, height: size.height))
		image.draw(at: CGPoint(x: 0,y: 0))
		let newImage = UIGraphicsGetImageFromCurrentImageContext()!
		UIGraphicsEndImageContext()
		return newImage
	}
	
	internal func imageWithMask(image: UIImage, mask: [CGRect]) -> UIImage {
		UIGraphicsBeginImageContextWithOptions(UITestsScreenshots.screenSize, false, 1)
		image.draw(at: CGPoint(x: 0, y: 0))
		_ = mask.map{UIRectFill($0)}
		let maskedImage = UIGraphicsGetImageFromCurrentImageContext()!
		UIGraphicsEndImageContext()
		return maskedImage
	}
	
	func convertForComparaison(screenshot: UIImage) -> UIImage {
		var area = getElementArea()
		if (svgManager.rects["mask"] == nil) {_=svgManager.parse()}
		var image = UITestsScreenshots.imageInScreenAcrea(image: screenshot, area: area)
		image = imageWithMask(image: image, mask: svgManager.rects["mask"]!)
		return image
	}
	
	func getElementArea() -> CGRect {
		XCTContext.runActivity(named: "get element coordinates") { _ in
			let area = element.frame
			let rect = CGRect(x: area.minX*3, y: area.minY*3, width: area.width*3, height: area.height*3)
			return rect
		}
	}
	
	//comparison functions
	static func getVarianceAreas(_ image1: UIImage, _ image2: UIImage) -> (image: UIImage, areas: [CGRect])? {
		
		let margin: CGFloat = 20
		let replacementColor: UInt8 = 255
		var areas = [CGRect]()
		
		//compare images
		UIGraphicsBeginImageContextWithOptions(image1.size, false, 1)
		setBackground(image: image1, color: UIColor.black).draw(at: CGPoint(x: 0, y: 0))
		setBackground(image: image2, color: UIColor.black).draw(at: CGPoint(x: 0, y: 0), blendMode: .difference, alpha: 1)
		let image = UIGraphicsGetImageFromCurrentImageContext()!
		UIGraphicsEndImageContext()
		
		//findRects
		let exeptMsg = "error! : unexpected error during image conversion for comparison"
		guard let inputCGImage = image.cgImage else {
			XCTFail(exeptMsg)
			return nil
		}
		
		let colorSpace       = CGColorSpaceCreateDeviceGray()
		let width            = inputCGImage.width
		let height           = inputCGImage.height
		let bytesPerPixel    = 1
		let bitsPerComponent = 8
		let bytesPerRow      = bytesPerPixel * width
		let bitmapInfo       = CGImageAlphaInfo.none.rawValue

		guard let context = CGContext(data: nil, width: width, height: height, bitsPerComponent: bitsPerComponent, bytesPerRow: bytesPerRow, space: colorSpace, bitmapInfo: bitmapInfo) else {
			XCTFail(exeptMsg)
			return nil
		}
		context.draw(inputCGImage, in: CGRect(x: 0, y: 0, width: width, height: height))
		
		guard let buffer = context.data else {
			XCTFail(exeptMsg)
			return nil
		}
		
		let pixelBuffer = buffer.bindMemory(to: UInt8.self, capacity: width * height)
		
		var rects: [Int:[CGRect]] = [:]

		for row in 0 ..< Int(height) {
			for column in 0 ..< Int(width) {
				let offset = row * width + column
				if (pixelBuffer[offset] > UInt8(colorTreshold)) {
					pixelBuffer[offset] = replacementColor
					let point = CGPoint(x: column, y: row)
					
					var rect = CGRect(x: point.x, y: point.y, width: 1, height: 1)
					for i in 0...1 {
						if let prevRects = rects[row-i] {
							for j in 0..<prevRects.count {
								if (rect.insetBy(dx: -1, dy: -1).intersects(prevRects[j])) {
									rect = prevRects[j].union(rect)
									rects[row-i]!.remove(at: j)
									break
								}
							}
						}
					}
					if (rects[row] == nil) {rects[row] = []}
					rects[row]!.append(rect)
					if (rect.width >= CGFloat(pixelTreshold) && rect.height >= CGFloat(pixelTreshold)) {
						areas.append(rect)
					}
					mergeCloseAreas(&areas, withMargin: margin)
					
				}
			}
		}
		
		let diff = UIImage(cgImage: (CGContext(data: pixelBuffer, width: width, height: height, bitsPerComponent: bitsPerComponent, bytesPerRow: bytesPerRow, space: colorSpace, bitmapInfo: bitmapInfo)?.makeImage())!)
		return (diff,areas)
	}
	
	private static func mergeCloseAreas(_ areas: inout [CGRect], withMargin margin: CGFloat) {
		guard (areas.count >= 1) else {return}
		let areaSize = areas.count
		for i in 1...areaSize {
			if (i != 1 && areas.last!.intersects(areas[areaSize-i].insetBy(dx: -margin, dy: -margin))) {
				areas[areas.count-1] = areas.last!.union(areas[areaSize-i])
				areas.remove(at: areaSize-i)
				mergeCloseAreas(&areas, withMargin: margin)
				break
			}
		}
	}
	
	//preview functions
		
	static func comparativePreview(reference: UIImage, screenshot: UIImage, difference: UIImage, areas: [CGRect]) -> UIImage {
		let realPreview = createPreview(title: "Real", image: screenshot, areas: areas, strokeColor: UIColor.red)
		let refPreview = createPreview(title: "Reference", image: reference, areas: areas, strokeColor: UIColor.red)
		let difPreview = createPreview(title: "Difference", image: difference)
		return createPreviewTable(images: refPreview,realPreview,difPreview)
	}
	
	static func variationPreview(reference: UIImage, areas: [CGRect]) -> UIImage {
		let variationPreview = createPreview(title: "Reference", image: reference, areas: areas, fillColor: UIColor.blue.withAlphaComponent(0.3),strokeColor: UIColor.blue)
		return variationPreview
	}
	
	static func createPreviewTable(images: UIImage...) -> UIImage {
		let sideMargin: CGFloat = 6
		let imageSize = images.first!.size
		let globalSize = CGSize(width: (imageSize.width+sideMargin)*CGFloat(images.count), height: imageSize.height+sideMargin)
		UIGraphicsBeginImageContextWithOptions(globalSize, false, 1)

		for i in 0...images.count-1 {
			images[i].draw(at: CGPoint(x: CGFloat(i)*(imageSize.width+sideMargin), y: sideMargin))
		}

		let tableImage = UIGraphicsGetImageFromCurrentImageContext()!
		UIGraphicsEndImageContext()
		return tableImage
	}
	
	static func createPreview(title: String, image: UIImage, areas: [CGRect] = [], fillColor: UIColor? = nil, strokeColor: UIColor? = nil) -> UIImage {
		let lineWidth: CGFloat = 3
		let bottomMargin = image.size.height*0.05
		let allSize = CGSize(width: image.size.width + lineWidth*2, height: image.size.height + bottomMargin + lineWidth*2)
		
		UIGraphicsBeginImageContextWithOptions(allSize, false, 1)
		
		//image
		image.draw(at: CGPoint(x: lineWidth, y: lineWidth), blendMode: .normal, alpha: 1)
		
		//rects
		UIGraphicsGetCurrentContext()?.setLineWidth(lineWidth)
		for area in areas {
			let newArea = area.offsetBy(dx: lineWidth, dy: lineWidth)
			if (fillColor != nil) {
				fillColor?.setFill()
				UIRectFillUsingBlendMode(newArea,.normal)
			}
			if (strokeColor != nil) {
				strokeColor?.setStroke()
				UIRectFrameUsingBlendMode(newArea.insetBy(dx: -3, dy: -3),.normal)
			}
		}
		
		//title
		let textFont = UIFont(name: "Helvetica Bold", size: bottomMargin/2)  ?? UIFont()
		let textStyle=NSMutableParagraphStyle()
		textStyle.alignment=NSTextAlignment.center
		let textColor = UIColor.black
		let textAttributes = [NSAttributedString.Key.font: textFont, NSAttributedString.Key.paragraphStyle: textStyle, NSAttributedString.Key.foregroundColor: textColor] as [NSAttributedString.Key : Any]
		let text_h = textFont.lineHeight
		let text_y = allSize.height-bottomMargin + (bottomMargin-text_h)/2
		let text_rect = CGRect(x: 0, y: text_y, width: allSize.width, height: bottomMargin)
		title.draw(in: text_rect, withAttributes: textAttributes)

		let preview = UIGraphicsGetImageFromCurrentImageContext()!
		UIGraphicsEndImageContext()
		return preview
	}
	
	//util functions
	
	private func referenceExist(expectedValue: Bool) -> Bool {
		guard (FileManager.default.fileExists(atPath: svgManager.path) == expectedValue) else {
			XCTFail(debugHistory + "error! : \(svgManager.path+(expectedValue ? " does not" : " already")) exist")
			return false
		}
		return true
	}
	
	private func referenceImagesExist(names: [String], expectedValue: Bool) -> Bool {
		guard svgManager.parse() else {return false}
		for name in names {
			let realName = viewName + (name != viewName ? "_"+name : "")
			if ((svgManager.images[realName] == nil) == expectedValue) {
				XCTFail(debugHistory + "error! : \(realName) image \(expectedValue ? "does not" : "already") exist")
				return false
			}
		}
		return true
	}
	
	private func checkVariationNonDefinition(caller: String = #function) -> Bool {
		guard variations.isEmpty else {
			XCTFail(debugHistory + "error : \"\(caller)\" function works only for views, not for views variations")
			return false
		}
		return true
	}
	
	//disk operation functions
	
	private func getPath(name: String) -> String{
		return "\(UITestsScreenshots.defaultPath)images/\(viewName+((name != viewName) ? "_"+name : "")).png"
	}
	
	private func saveImage(image: UIImage, path: String) {
		guard let data = image.pngData() else {
			XCTFail("error != unable to save image at \(path)")
			return
		}
		do {
			try data.write(to: URL(fileURLWithPath: path))
		} catch {
			NSLog(error.localizedDescription)
			XCTFail(debugHistory + "error != unable to save image at \(path)")
		}
	}
	
	private func getImage(path: String) -> UIImage? {
		guard let image = UIImage(contentsOfFile: path) else {
			XCTFail(debugHistory + "error != unable to get image at \(path)")
			return nil
		}
		return image
	}
	
}


class SVGManager : NSObject {
	var path: String
	var parentPath: String
	var rects: [String : [CGRect]] = [:]
	var images: [String : (area: CGRect, line: Int)] = [:]
	private var parser: XMLParser?
	private var current: String?
	
	private var defaultMask = [CGRect(x: 376, y: 2492, width: 418, height: 16)]
	
	private var referenceStart: Int?
	private var maskStart: Int?
	private var clipPathStart: Int?
	private var variationsStart: Int?
	private var selectedVariationStarts: [String : Int]?
	
	init(path: String) {
		self.path = path
		let index = path.lastIndex(of: "/")
		self.parentPath = (index != nil) ? String(path.prefix(path.distance(from: path.startIndex, to: index!)+1)) : ""
		
	}
	
	func svgRect(_ rect: CGRect, lock: Bool) -> String {
		return "<rect x=\"\(rect.minX)\" y=\"\(rect.minY)\" width=\"\(rect.width)\" height=\"\(rect.height)\"\(lock ? " sodipodi:insensitive=\"true\"":"")/>"
	}
	
	func svgImage(name: String, area: CGRect, lock: Bool) -> String {
		return "<image id=\"\(name)\" x=\"\(area.minX)\" y=\"\(area.minY)\" width=\"\(area.width)\" height=\"\(area.height)\" preserveAspectRatio=\"none\" xlink:href=\"images/\(name).png\"\(lock ? " sodipodi:insensitive=\"true\"":"")/>"
	}
	
	func createFile(referenceName name: String, referenceArea area: CGRect, svgSize: CGSize) {
		let width = svgSize.width
		let height = svgSize.height
		var content = [String]()
		content.append("<svg width=\"\(width)\" height=\"\(height)\" version=\"1.1\" viewBox=\"0 0 \(width) \(height)\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:sodipodi=\"http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd\">")
		content.append("<g id=\"reference\" sodipodi:insensitive=\"true\">")
		content.append("   \(svgImage(name: name, area: area, lock: true))")
		content.append("</g>")
		content.append("<defs>\n   <clipPath id=\"clipPath\"/>\n</defs>")
		content.append("<g id=\"variations\" clip-path=\"url(#clipPath)\"/>")
		content.append("<g id=\"mask\" opacity=\"0.4\" fill=\"#000\" stroke=\"none\">")
		_ = defaultMask.map{content.append("   \(svgRect($0, lock: true))")}
		content.append("</g>")
		content.append("</svg>")
		let svg = content.joined(separator: "\n")
		try! svg.write(toFile: path, atomically: true, encoding: String.Encoding.utf8)
	}
	
	func updateImage(name: String, area: CGRect) -> Bool {
		guard parse(), images[name] != nil else {return false}
		
		var svgData = try! String(contentsOf: URL(fileURLWithPath: path)).split(separator: "\n")
		var lineIndex = images[name]!.line-1
		var line = svgData[lineIndex]
		while line.firstIndex(of: "<") == nil {
			svgData.remove(at: lineIndex)
			lineIndex += -1
			line = svgData[lineIndex]
		}
		svgData[lineIndex] = line.prefix(line.distance(from: line.startIndex, to: line.firstIndex(of: "<")!)) + Substring(svgImage(name: name, area: area, lock: true))
		try! String(svgData.joined(separator: "\n")).write(toFile: path, atomically: true, encoding: String.Encoding.utf8)
		return true
	}
	
	func addToMask(rects: [CGRect]) -> Bool {
		guard parse() else {return false}
		var content = ""
		for rect in rects {
			content += "\(svgRect(rect, lock: false))\n"
		}
		svgInsert(content, at: maskStart!)
		return true
	}
	
	func addVariation(referenceName: String, name: String, area: CGRect, rects: [CGRect]) -> Bool {
		guard parse() else {return false}
		
		var content = "<g id=\"\(name)\" display=\"none\" fill=\"none\" stroke=\"#00f\" stroke-width=\"3\">\n"
		content += "   \(svgImage(name: "\(referenceName)_\(name)", area: area, lock: true))\n"
		for rect in rects {
			content += "   \(svgRect(rect, lock: false))\n"
		}
		content += "</g>\n"
		svgInsert(content, at: variationsStart!)
		 
		content = "<use xlink:href=\"#\(name)\"/>"
		svgInsert(content, at: clipPathStart!, parentElement: "clipPath")
		return true
	}
	
	//if some variations are delete with a svg editor like Inkscape, this variations are copied in the clipPath wich break some functionalities.
	//this function is called at eeach parse to clean the clipPath if needed.
	func removeClipPathAnomalies() {
		guard (clipPathStart ?? 0 < (variationsStart ?? 0 )-2) else {return}
		var svgData = try! String(contentsOf: URL(fileURLWithPath: path)).split(separator: "\n")
		var index = clipPathStart!
		var use = false
		var line = svgData[index]
		while !line.contains("</clipPath>") && !line.contains("</defs>") {
			if (use == true) {
				if (line.contains("/>")) {use = false}
			} else {
				if (line.range(of: "<use") != nil) {use = true}
				else {
					svgData.remove(at: index)
					index += -1
				}
			}
			index += 1
			line = svgData[index]
		}
		try! String(svgData.joined(separator: "\n")).write(toFile: path, atomically: true, encoding: String.Encoding.utf8)
	}
	
	func parse(withVariations headers: [String] = []) -> Bool {
		rects["mask"] = []
		_ = headers.map{rects[$0] = []}
		parser = XMLParser(contentsOf: URL(fileURLWithPath: path))
		guard (parser != nil) else {return false}
		parser!.delegate = self
		parser!.parse()
		removeClipPathAnomalies()
		return true
	}
	
	func svgInsert(_ content: String, at index: Int, parentElement: String = "g") {
		var svgData = try! String(contentsOf: URL(fileURLWithPath: path)).split(separator: "\n")
		let line = svgData[index-1]
		let prefix = line.prefix(line.distance(from: line.startIndex, to: line.firstIndex(of: "<") ?? line.startIndex))
		if (line.suffix(2) == "/>") {
			svgData[index-1] = line.prefix(line.count-2)+">"
			svgData.insert(prefix+"</\(parentElement)>", at: index)
		}
		svgData.insert(prefix+Substring("   "+content), at: index)

		try! String(svgData.joined(separator: "\n")).write(toFile: path, atomically: true, encoding: String.Encoding.utf8)
	}
}

extension SVGManager : XMLParserDelegate {
	
	func parser(_ parser: XMLParser, didStartElement elementName: String, namespaceURI: String?, qualifiedName qName: String?, attributes attributeDict: [String : String] = [:]) {

		if (elementName == "g") {
			if (rects[attributeDict["id"] ?? ""] != nil) {
				current = attributeDict["id"]
			}
			if (attributeDict["id"] == "reference") {
				referenceStart = parser.lineNumber
			} else if (attributeDict["id"] == "mask") {
				maskStart = parser.lineNumber
			} else if (attributeDict["id"] == "variations") {
				variationsStart = parser.lineNumber
			}
		}
		
		if (elementName == "clipPath" && attributeDict["id"] == "clipPath") {
			clipPathStart = parser.lineNumber
		}
		
		if (elementName == "rect" && current != nil) {
			if current != nil { rects[current!]?.append(getRect(attributes: attributeDict))}
		}
		
		if (elementName == "image") {
			guard var imageName = attributeDict["xlink:href"] else {return}
			guard FileManager.default.fileExists(atPath: imageName) || FileManager.default.fileExists(atPath: parentPath+imageName) else {
				return
			}
			imageName = String(imageName.suffix(imageName.distance(from: imageName.lastIndex(of: "/") ?? imageName.startIndex, to: imageName.endIndex)-1))
			imageName = String(imageName.prefix(imageName.distance(from: imageName.startIndex, to: imageName.lastIndex(of: ".") ?? imageName.endIndex)))
			images[imageName] = (getRect(attributes: attributeDict),parser.lineNumber)
		}
	}
	
	func parser(_ parser: XMLParser, didEndElement elementName: String, namespaceURI: String?, qualifiedName qName: String?) {
		if (elementName == "g") {
			current = nil
		}
	}
	
	private func getRect(attributes: [String:String]) -> CGRect {
		var values = [Int]()
		for header in ["x","y","width","height"] {
			if let val = NumberFormatter().number(from: attributes[header] ?? "0") {
				values.append(Int(truncating: val))
			}
		}
		return CGRect(x: values[0], y: values[1], width: values[2], height: values[3])
	}
}
