//
//  MyCell.swift
//  linphone
//
//  Created by Benoît Martins on 13/12/2022.
//

import UIKit
import DropDown

class MyCell: DropDownCell {
	
	@IBOutlet var myImageView: UIImageView!

    override func awakeFromNib() {
        super.awakeFromNib()
		myImageView.contentMode = .scaleAspectFit
    }

    override func setSelected(_ selected: Bool, animated: Bool) {
        super.setSelected(selected, animated: animated)

        // Configure the view for the selected state
    }
    
}
