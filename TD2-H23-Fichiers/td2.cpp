#pragma region "Includes"//{
#define _CRT_SECURE_NO_WARNINGS // On permet d'utiliser les fonctions de copies de chaînes qui sont considérées non sécuritaires.

#include "structures.hpp"      // Structures de données pour la collection de films en mémoire.

#include <iostream>
#include <fstream>
#include <string>
#include <limits>
#include <algorithm>

#include "include/cppitertools/range.hpp"
#include "include/gsl/span"

/*#include "bibliotheque_cours.hpp"
#include "verification_allocation.hpp" // Nos fonctions pour le rapport de fuites de mémoire.
#include "debogage_memoire.hpp"*/      // Ajout des numéros de ligne des "new" dans le rapport de fuites.  Doit être après les include du système, qui peuvent utiliser des "placement new" (non supporté par notre ajout de numéros de lignes).

using namespace std;
using namespace iter;
using namespace gsl;

#pragma endregion//}

typedef uint8_t UInt8;
typedef uint16_t UInt16;

#pragma region "Fonctions de base pour lire le fichier binaire"//{

UInt8 lireUint8(istream& fichier)
{
	UInt8 valeur = 0;
	fichier.read((char*)&valeur, sizeof(valeur));
	return valeur;
}
UInt16 lireUint16(istream& fichier)
{
	UInt16 valeur = 0;
    UInt16 val = sizeof(valeur);
	fichier.read((char*)&valeur, sizeof(valeur));
	return valeur;
}
string lireString(istream& fichier)
{
	string texte;
	texte.resize(lireUint16(fichier));
	fichier.read((char*)&texte[0], streamsize(sizeof(texte[0])) * texte.length());
	return texte;
}

#pragma endregion//}

//TODO: Une fonction pour ajouter un Film à une ListeFilms, le film existant déjà; on veut uniquement ajouter le pointeur vers le film existant.  Cette fonction doit doubler la taille du tableau alloué, avec au minimum un élément, dans le cas où la capacité est insuffisante pour ajouter l'élément.  Il faut alors allouer un nouveau tableau plus grand, copier ce qu'il y avait dans l'ancien, et éliminer l'ancien trop petit.  Cette fonction ne doit copier aucun Film ni Acteur, elle doit copier uniquement des pointeurs.
void addMovie(Film* movie, ListeFilms& movieLib){
    if (movieLib.capacite == movieLib.nElements) {
        int newCap;
        if (movieLib.capacite == 0) newCap = 1;
        else newCap = movieLib.capacite * 2;

        Film ** movies = new Film * [newCap];
        for (int i = 0; i < movieLib.nElements; i++) {
            movies[i] = movieLib.elements[i];
            int k = 0;
        }
        delete[] movieLib.elements;
        movieLib.elements = movies;
        movieLib.capacite = newCap;
    }

    movieLib.elements[movieLib.nElements++] = movie;
}

//TODO: Une fonction pour enlever un Film d'une ListeFilms (enlever le pointeur) sans effacer le film; la fonction prenant en paramètre un pointeur vers le film à enlever.  L'ordre des films dans la liste n'a pas à être conservé.
void removeMovie(Film* movie, ListeFilms& movieLib){
    for (int i = 0; i < movieLib.nElements; i ++){
        if (movieLib.elements[i] == movie) {
            movieLib.elements[i] = nullptr;
        }
    }
}

//TODO: Une fonction pour trouver un Acteur par son nom dans une ListeFilms, qui retourne un pointeur vers l'acteur, ou nullptr si l'acteur n'est pas trouvé.  Devrait utiliser span.
Acteur* findActor(const string& bobsName, ListeFilms& movieLib){
    for (int i = 0; i < movieLib.nElements; i++){
        if (movieLib.elements[i] != nullptr){
            for (int k = 0; k < movieLib.elements[i]->acteurs.nElements; k++){
                if (movieLib.elements[i]->acteurs.elements[k]->nom == bobsName){
                    return movieLib.elements[i]->acteurs.elements[k];
                }
            }
        }
    }
    return nullptr;
}

//TODO: Compléter les fonctions pour lire le fichier et créer/allouer une ListeFilms.  La ListeFilms devra être passée entre les fonctions, pour vérifier l'existence d'un Acteur avant de l'allouer à nouveau (cherché par nom en utilisant la fonction ci-dessus).
Acteur* lireActeur(istream& fichier, ListeFilms& movieLib)
{
	Acteur acteur = {};
	acteur.nom            = lireString(fichier);
	acteur.anneeNaissance = lireUint16 (fichier);
	acteur.sexe           = lireUint8  (fichier);
    Acteur* exists = findActor(acteur.nom, movieLib);
    if (exists == nullptr) {
        auto act = new Acteur;
        *act = acteur;
        return act;
    };
    return exists;
	 //TODO: Retourner un pointeur soit vers un acteur existant ou un nouvel acteur ayant les bonnes informations, selon si l'acteur existait déjà.  Pour fins de débogage, affichez les noms des acteurs crées; vous ne devriez pas voir le même nom d'acteur affiché deux fois pour la création.
}

Film* lireFilm(istream& fichier, ListeFilms& movieLib)
{
	Film film = {};
	film.titre       = lireString(fichier);
	film.realisateur = lireString(fichier);
	film.anneeSortie = lireUint16 (fichier);
	film.recette     = lireUint16 (fichier);
	film.acteurs.nElements = lireUint8 (fichier);  //NOTE: Vous avez le droit d'allouer d'un coup le tableau pour les acteurs, sans faire de réallocation comme pour ListeFilms.  Vous pouvez aussi copier-coller les fonctions d'allocation de ListeFilms ci-dessus dans des nouvelles fonctions et faire un remplacement de Film par Acteur, pour réutiliser cette réallocation.

    Acteur** tableauDActeurs = new Acteur * [film.acteurs.nElements];
    film.acteurs.capacite = film.acteurs.nElements;
    film.acteurs.elements = tableauDActeurs;

    //TODO: Ajouter le film à la liste des films dans lesquels l'acteur joue.

    for (int i : range(0, film.acteurs.nElements)){
        film.acteurs.elements[i] = lireActeur(fichier, movieLib);
        if (film.acteurs.elements[i]->joueDans.nElements == 0){
            Film ** joueDans = new Film * [film.acteurs.elements[i]->joueDans.capacite];
            film.acteurs.elements[i]->joueDans = ListeFilms{0, 0, joueDans};
        }
        addMovie(&film, film.acteurs.elements[i]->joueDans);
    }

    Film* f = new Film;
    *f = film;
	return f; //TODO: Retourner le pointeur vers le nouveau film.
}

ListeFilms creerListe(const string& nomFichier)
{
	ifstream fichier(nomFichier, ios::binary);
	fichier.exceptions(ios::failbit);

	int nElements = lireUint16(fichier);

	//TODO: Créer une liste de films vide.
    Film ** filmsInit = new Film * [0];
    ListeFilms films {0, 0, filmsInit};
    //TODO: Ajouter le film à la liste.

    for (int i : range(0, nElements)){
        auto temp = lireFilm(fichier, films);
        cout<<temp->titre;
        addMovie(temp, films);
        int k = 0;
    }

    //TODO: Retourner la liste de films.
    return films;
}

//TODO: Une fonction pour détruire un film (relâcher toute la mémoire associée à ce film, et les acteurs qui ne jouent plus dans aucun films de la collection).  Noter qu'il faut enlever le film détruit des films dans lesquels jouent les acteurs.  Pour fins de débogage, affichez les noms des acteurs lors de leur destruction.

void destroyMovie(ListeFilms& movieLib, Film* film){
    for (int i: range(0, movieLib.nElements)){
        if (movieLib.elements[i] == film){
            // movie found: deleting it completely
            // delete actors
            for (int k : range(0, movieLib.elements[i]->acteurs.nElements)){
                if (movieLib.elements[i]->acteurs.elements[k]->joueDans.nElements == 1){
                    delete[] movieLib.elements[i]->acteurs.elements[k]->joueDans.elements[0];
                    delete[] movieLib.elements[i]->acteurs.elements[k];
                } else delete movieLib.elements[i]->acteurs.elements[k];
            }
            // delete movie
            delete[] movieLib.elements[i];
        }
    }
}

//TODO: Une fonction pour détruire une ListeFilms et tous les films qu'elle contient.

void afficherActeur(const Acteur& acteur)
{
	cout << "  " << acteur.nom << ", " << acteur.anneeNaissance << " " << acteur.sexe << endl;
}

//TODO: Une fonction pour afficher un film avec tous ces acteurs (en utilisant la fonction afficherActeur ci-dessus).

void afficherListeFilms(const ListeFilms& listeFilms)
{
	//TODO: Utiliser des caractères Unicode pour définir la ligne de séparation (différente des autres lignes de séparations dans ce progamme).
	static const string ligneDeSeparation = {};
	cout << ligneDeSeparation;
	//TODO: Changer le for pour utiliser un span.
	for (int i = 0; i < listeFilms.nElements; i++) {
		//TODO: Afficher le film.
		cout << ligneDeSeparation;
	}
}

void afficherFilmographieActeur(const ListeFilms& listeFilms, const string& nomActeur)
{
	//TODO: Utiliser votre fonction pour trouver l'acteur (au lieu de le mettre à nullptr).
	const Acteur* acteur = nullptr;
	if (acteur == nullptr)
		cout << "Aucun acteur de ce nom" << endl;
	else
		afficherListeFilms(acteur->joueDans);
}

int main()
{
	// bibliotheque_cours::activerCouleursAnsi();  // Permet sous Windows les "ANSI escape code" pour changer de couleurs https://en.wikipedia.org/wiki/ANSI_escape_code ; les consoles Linux/Mac les supportent normalement par défaut.

    // int* fuite = new int; //TODO: Enlever cette ligne après avoir vérifié qu'il y a bien un "Fuite detectee" de "4 octets" affiché à la fin de l'exécution, qui réfère à cette ligne du programme.

	static const string ligneDeSeparation = "\n\033[35m════════════════════════════════════════\033[0m\n";

    Film** movies = new Film * ;

    ListeFilms library {0,  0, movies};

	//TODO: Chaque TODO dans cette fonction devrait se faire en 1 ou 2 lignes, en appelant les fonctions écrites.

	//TODO: La ligne suivante devrait lire le fichier binaire en allouant la mémoire nécessaire.  Devrait afficher les noms de 20 acteurs sans doublons (par l'affichage pour fins de débogage dans votre fonction lireActeur).
	ListeFilms listeFilms = creerListe("/Users/emirtuncbilek/Desktop/tp3/TD2-H23-Fichiers/films.bin");
    for (int i = 0; i < listeFilms.nElements; i++){
        if (listeFilms.elements[i] != nullptr) cout<<listeFilms.elements[i]->titre <<"\n";
    }


	cout << ligneDeSeparation << "Le premier film de la liste est:" << endl;

	//TODO: Afficher le premier film de la liste.  Devrait être Alien.

	cout << ligneDeSeparation << "Les films sont:" << endl;
	//TODO: Afficher la liste des films.  Il devrait y en avoir 7.


	//TODO: Modifier l'année de naissance de Benedict Cumberbatch pour être 1976 (elle était 0 dans les données lues du fichier).  Vous ne pouvez pas supposer l'ordre des films et des acteurs dans les listes, il faut y aller par son nom.
	
	cout << ligneDeSeparation << "Liste des films où Benedict Cumberbatch joue sont:" << endl;
	//TODO: Afficher la liste des films où Benedict Cumberbatch joue.  Il devrait y avoir Le Hobbit et Le jeu de l'imitation.
	
	//TODO: Détruire et enlever le premier film de la liste (Alien).  Ceci devrait "automatiquement" (par ce que font vos fonctions) détruire les acteurs Tom Skerritt et John Hurt, mais pas Sigourney Weaver puisqu'elle joue aussi dans Avatar.
	
	cout << ligneDeSeparation << "Les films sont maintenant:" << endl;
	//TODO: Afficher la liste des films.
	
	//TODO: Faire les appels qui manquent pour avoir 0% de lignes non exécutées dans le programme (aucune ligne rouge dans la couverture de code; c'est normal que les lignes de "new" et "delete" soient jaunes).  Vous avez aussi le droit d'effacer les lignes du programmes qui ne sont pas exécutée, si finalement vous pensez qu'elle ne sont pas utiles.
	
	//TODO: Détruire tout avant de terminer le programme.  La bibliothèque de verification_allocation devrait afficher "Aucune fuite detectee." a la sortie du programme; il affichera "Fuite detectee:" avec la liste des blocs, s'il manque des deletes.

}
