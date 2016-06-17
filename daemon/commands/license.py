import glob, os


os.chdir("./")
for fichier in glob.glob("*.cc"):
    print(fichier)
    f = open(fichier)
    text=f.read()
    f.close()
    seq = ["/*\n",fichier ,"\nCopyright (C) 2016 Belledonne Communications, Grenoble, France \n",\
    "\nThis library is free software; you can redistribute it and/or modify it\n",\
    "under the terms of the GNU Lesser General Public License as published by\n",\
    "the Free Software Foundation; either version 2.1 of the License, or (at\n",\
    "your option) any later version.\n",\
    "\nThis library is distributed in the hope that it will be useful, but WITHOUT\n",\
    "ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or\n",\
    "FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public\nLicense for more details.\n",\
    "\nYou should have received a copy of the GNU Lesser General Public License\n",\
    "along with this library; if not, write to the Free Software Foundation," \
	"\nInc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA\n" \
	"*/\n\n"]
    f = open(fichier,'wb')
    f.seek(0)
    line = f.writelines(seq)
    f.write(text)
    f.close()

