# Author:   H. van der Westhuizen, u18141235
# Date:     28 February 2020
# Revision: 29 February 2020
# Genetic algorithm.

# Data set 1 output sequence:
# P,S,S,P,R,S,P,P,R,R,S,P,S,S,P,P,S,R,R,S,R,S,R,R,S,P,R,P,R,S,P,P,R,P,S,P,P,S,S,R,S,R,P,
# S,P,P,S,R,P,R,R,R,R,R,P,S,R,P,P,P,P,R,R,S,S,S,R,S,S,P,S,R,S,S,R,R,P,R,P,P,R
# Data set 2 output sequence:
# P,S,P,P,R,R,P,P,P,R,S,P,S,S,P,P,S,S,R,S,R,R,R,R,S,P,R,P,P,S,P,P,R,P,P,P,R,S,S,P,S,R,
# R,S,P,R,R,R,S,R,R,R,R,R,P,R,R,P,S,S,P,P,R,S,S,S,R,S,S,P,S,S,S,R,R,R,P,R,P,S,R

import csv
import random
from operator import attrgetter
from matplotlib import pyplot as plt

optimal_CSV = []  # This array holds the compressed 81 length sequence of the data sets.
pop_size = 100  # This is the population size of each generation.
population = []  # This array holds the child objects of each individual within the current generation.
beat = {"R": "P", "P": "S", "S": "R"}  # This is used to determine the best move to play against the opponent.
bestChildArray = []  # This array holds the fittest child for every generation in array of size: total generations.Plot
averageChildArray = []  # This array holds average fitness of a population every generation.(Plot)


# The class below creates the object of the child/individual. It holds the individuals fitness, gene sequence and
# index.
class child():
    def __init__(self, childIndex=0):
        self.fitness = 0  # This is the total fitness of the child/ individual.
        self.gene_sequence = []  # This is the child gene sequence of length 81.
        self.childIndex = childIndex  # This index is used to keep track of certain individuals and used in sorting.


# This dictionary is used to read from the CSV files during the construction of the optimal_CSV array.
dictionary = [['R', 'R', 'R', 'R'], ['R', 'R', 'R', 'P'],
              ['R', 'R', 'R', 'S'], ['R', 'R', 'P', 'R'],
              ['R', 'R', 'P', 'P'], ['R', 'R', 'P', 'S'],
              ['R', 'R', 'S', 'R'], ['R', 'R', 'S', 'P'],
              ['R', 'R', 'S', 'S'], ['R', 'P', 'R', 'R'],
              ['R', 'P', 'R', 'P'], ['R', 'P', 'R', 'S'],
              ['R', 'P', 'P', 'R'], ['R', 'P', 'P', 'P'],
              ['R', 'P', 'P', 'S'], ['R', 'P', 'S', 'R'],
              ['R', 'P', 'S', 'P'], ['R', 'P', 'S', 'S'],
              ['R', 'S', 'R', 'R'], ['R', 'S', 'R', 'P'],
              ['R', 'S', 'R', 'S'], ['R', 'S', 'P', 'R'],
              ['R', 'S', 'P', 'P'], ['R', 'S', 'P', 'S'],
              ['R', 'S', 'S', 'R'], ['R', 'S', 'S', 'P'],
              ['R', 'S', 'S', 'S'], ['P', 'R', 'R', 'R'],
              ['P', 'R', 'R', 'P'], ['P', 'R', 'R', 'S'],
              ['P', 'R', 'P', 'R'], ['P', 'R', 'P', 'P'],
              ['P', 'R', 'P', 'S'], ['P', 'R', 'S', 'R'],
              ['P', 'R', 'S', 'P'], ['P', 'R', 'S', 'S'],
              ['P', 'P', 'R', 'R'], ['P', 'P', 'R', 'P'],
              ['P', 'P', 'R', 'S'], ['P', 'P', 'P', 'R'],
              ['P', 'P', 'P', 'P'], ['P', 'P', 'P', 'S'],
              ['P', 'P', 'S', 'R'], ['P', 'P', 'S', 'P'],
              ['P', 'P', 'S', 'S'], ['P', 'S', 'R', 'R'],
              ['P', 'S', 'R', 'P'], ['P', 'S', 'R', 'S'],
              ['P', 'S', 'P', 'R'], ['P', 'S', 'P', 'P'],
              ['P', 'S', 'P', 'S'], ['P', 'S', 'S', 'R'],
              ['P', 'S', 'S', 'P'], ['P', 'S', 'S', 'S'],
              ['S', 'R', 'R', 'R'], ['S', 'R', 'R', 'P'],
              ['S', 'R', 'R', 'S'], ['S', 'R', 'P', 'R'],
              ['S', 'R', 'P', 'P'], ['S', 'R', 'P', 'S'],
              ['S', 'R', 'S', 'R'], ['S', 'R', 'S', 'P'],
              ['S', 'R', 'S', 'S'], ['S', 'P', 'R', 'R'],
              ['S', 'P', 'R', 'P'], ['S', 'P', 'R', 'S'],
              ['S', 'P', 'P', 'R'], ['S', 'P', 'P', 'P'],
              ['S', 'P', 'P', 'S'], ['S', 'P', 'S', 'R'],
              ['S', 'P', 'S', 'P'], ['S', 'P', 'S', 'S'],
              ['S', 'S', 'R', 'R'], ['S', 'S', 'R', 'P'],
              ['S', 'S', 'R', 'S'], ['S', 'S', 'P', 'R'],
              ['S', 'S', 'P', 'P'], ['S', 'S', 'P', 'S'],
              ['S', 'S', 'S', 'R'], ['S', 'S', 'S', 'P'],
              ['S', 'S', 'S', 'S']]


# The function below compresses the information in the data sets of 1 and 2. This allows for less overhead and
# ease of comparison. Ultimately the compressed array holds an 81 gene sequence that will help in determining
# the fitness of the individuals.
def compressData():
    # This for loop creates the 81 gene array containing the csv file, History and amount of a response, R=0 , P=0 ,S=0
    global optimal_CSV
    for i in range(81):
        optimal_CSV.append([dictionary[i], 0, 0, 0])
    # The code below allows the code too read from the data CSV files row by row which is divided by (,).
    with open('data1.csv') as csv_data:
        readCSV = (csv.reader(csv_data, delimiter=','))
        # The for loop below goes through all of the rows (1000000) and counts the R,P and S resp for each history.
        for row in readCSV:
            CSV_History = list(row[0])  # This variable stores the History (last four moves) such as RRRR.
            CSV_Response = list(row[1])[0]  # This variable stores the Response given the history such as R or P.
            # The if statements below check the history and increment the R,P,S counter for the given history and resp
            # For example [RRRR,9000,200,0].The given history thus shows the responses were 9000 R, 200 P and 0 S.
            if CSV_Response == 'R':
                optimal_CSV[dictionary.index(CSV_History)][1] = optimal_CSV[dictionary.index(CSV_History)][1] + 1
            elif CSV_Response == 'P':
                optimal_CSV[dictionary.index(CSV_History)][2] = optimal_CSV[dictionary.index(CSV_History)][2] + 1
            else:
                optimal_CSV[dictionary.index(CSV_History)][3] = optimal_CSV[dictionary.index(CSV_History)][3] + 1
    # The for loop below now just creates the 81 length gene gathered from the CSV file, this will be used in the
    # calculation of the GA's fitness. It is constructed according to the amount of R,P and S responses were noted
    # per history.
    for i in range(81):
        if (optimal_CSV[i][1] > optimal_CSV[i][2]) and (optimal_CSV[i][1] > optimal_CSV[i][3]):
            optimal_CSV[i] = 'R'
        elif (optimal_CSV[i][2] > optimal_CSV[i][1]) and (optimal_CSV[i][2] > optimal_CSV[i][3]):
            optimal_CSV[i] = 'P'
        elif (optimal_CSV[i][3] > optimal_CSV[i][1]) and (optimal_CSV[i][3] > optimal_CSV[i][2]):
            optimal_CSV[i] = 'S'
        else:
            optimal_CSV[i] = random.choice(["R", "P", "S"])  # Not all histories are represented thus its randomised.


# The function below initialises the population by creating random children/individuals that will represent
# the first generation. This is done by randomising their gene sequences.
def initialisePopulation():
    global pop_size  # This is the population size (number of children)
    global population
    #  Below the population is created with the size of pop_size and consists of child objects.
    population = [child() for i in range(pop_size)]
    # The for loop below initialises the entire population by randomising there gene_sequence and clears fitness.
    for x in range(len(population)):
        for y in range(81):
            population[x].gene_sequence.append(random.choice(["R", "P", "S"]))
        population[x].fitness = 0
        population[x].childIndex = x


# This function is used to calculate each child/individuals fitness by comparing there entire gene sequence to the
# optimal_CSV gene sequence. The population is then sorted according to there fitness which will add the crossover
# and mutation stages.
def calculateFitness():
    global pop_size
    global bestChildArray
    global averageChildArray
    global population
    averageChildCounter = 0
    # The for loop below calculates the fitness of each child. By comparing each of the child's genes to the "optimal
    # gene" (optimal_CSV) in the sequence.
    for x in range(pop_size):
        fitnessCounter = 0  # This variable keeps track of the child's fitness.
        for y in range(81):
            # If the child's gene beats the optimal gene then it increases its fitness by 1 and if it is beaten it
            # decreases by 1.
            if population[x].gene_sequence[y] == beat[optimal_CSV[y]]:
                fitnessCounter += 1
            elif optimal_CSV[y] == beat[population[x].gene_sequence[y]]:
                fitnessCounter -= 1
        population[x].fitness = fitnessCounter
    # Here the population is ranked and sorted by their fitness. It is ranked from best to worst. This process will
    # aid during crossover and mutation.
    population = sorted(population, key=attrgetter("fitness"), reverse=True)
    bestChildCounter = population[0].fitness  # The fittest individual is determined.(Plot)
    # Below the average population fitness is determined.(Plot)
    for x in range(pop_size):
        population[x].childIndex = x
        if bestChildCounter < population[x].fitness:
            bestChildCounter = population[x].fitness
        averageChildCounter += population[x].fitness
    averageChildCounter = int(averageChildCounter / pop_size)
    bestChildArray.append(bestChildCounter)
    averageChildArray.append(averageChildCounter)


# The function below does the bulk of the genetic algorithm implementation as it is in charge of crossover, mutation
# and reproduction of the new generation. The system is split into 3 sections elite, products of crossover and mutation
# and random population insertion.
def crossover_mutation():
    global population
    global pop_size
    # This variable keeps track of the portion of the population already accounted for, ensures stable population size
    popAccounted = 0
    mutation_rate = (5 / 100)  # This is the percentage chance that a gene of the sequence will mutate
    elite_percentage = (10 / 100)  # This is the percentage of parents that will stay unaltered into the next generation
    COC = (70 / 100)  # This is the amount of children that are products of crossover and potential mutation
    crossover_point = [int(81 / 3), 40, int((2 * 81) / 3)]  # This array hold the three possible crossover points
    tempPopulation = []  # This array hold the children of the new generation as it is produced.
    tempChild = child()
    tempChild.gene_sequence = ["X"] * 81
    # The popAccounted now accounts for the elite individuals.
    popAccounted += int(pop_size * elite_percentage)
    # The elite individuals are transferred to the next generation as is with no alterations. Small portion of pop.
    for i in range(int(pop_size * elite_percentage)):
        tempPopulation.append(population[i])
    # The popAccounted accounts for the elite individuals and individuals that are products of crossover and mutation.
    popAccounted += int(pop_size * COC)
    # The loop below creates the proportion of the population that is a product of crossover and mutation.
    # The elite individuals act as the parents, 2 random elite individuals are crossovered together to make a COC child
    for i in range(int(pop_size * COC)):
        tempChild = child()
        tempChild.gene_sequence = ["X"] * 81
        parent1 = tempPopulation[random.randrange(int(pop_size * elite_percentage))]
        parent2 = tempPopulation[random.randrange(int(pop_size * elite_percentage))]
        while (parent1 == parent2):
            parent2 = tempPopulation[random.randrange(int(pop_size * elite_percentage))]
        point_cross = crossover_point[random.randrange(len(crossover_point))]
        countPosition = 0
        # Cross over takes place at 1 of three positions. The parent 1 fills in from one side then second parent
        # fills in the rest.
        for x in range(81):
            if countPosition <= point_cross:
                tempChild.gene_sequence[x] = parent1.gene_sequence[x]
            else:
                tempChild.gene_sequence[x] = parent2.gene_sequence[x]
            # During every gene crossover the gene has a set chance of mutating into any of the tree options,
            # even the one it currently is thus it has (mutation rate/100)*0.667 chance of changing.
            if random.random() < mutation_rate:
                tempChild.gene_sequence[x] = random.choice(['R', 'P', 'S'])
            countPosition += 1
        tempPopulation.append(tempChild)
    # The remaining population is then a product of pure randomness, they are basically reinitialised. This ensures
    # that it will never get stuck in local minimum and will always converge to global maximum even if it take
    # very long.
    for i in range(pop_size - popAccounted):
        tempChild = child()
        tempChild.gene_sequence = ["X"] * 81
        for x in range(81):
            tempChild.gene_sequence[x] = random.choice(['R', 'P', 'S'])
        tempPopulation.append(tempChild)
    population = tempPopulation

# The program starts here. (entry point)
if __name__ == "__main__":
    compressData()  # The CSV data set is compressed.
    initialisePopulation()  # The population is initialised
    calculateFitness()  # The 1st generation is ranked and indexed. (There fitness is gathered)
    tick = 1 # Keeps track of the generation.
    generationArray = [tick]  # Helps with plotting fitness vs generation.
    # The program runs until there is a individual with a fitness equal to or greater than the one desired and
    # specified in the while loop.
    while population[0].fitness <= 80:
        tick += 1
        crossover_mutation()  # Each generation mutation takes place
        calculateFitness()  # Each generation the individuals are ranked.
        generationArray.append(tick)  # increase the generation.
    file = open("Data1.txt", "w+")
    for i in range(0, 81):
        file.write(population[0].gene_sequence[i])
        if i != 80:
            file.write(",")
    plt.plot(generationArray, bestChildArray, label="Fittest individual")
    plt.plot(generationArray, averageChildArray, label="Average population fitness")
    plt.xlabel('Generation')
    plt.ylabel('Fittest')
    plt.legend()
    plt.grid(color='black', linewidth=0.5)
    plt.savefig('Graph_Constant10.png')
    plt.show()
